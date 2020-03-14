#include "web.h"

#include "index.h"
#include "config.h"
#include "state.h"

#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

ESP8266WebServer server;

void handleGetIndex()
{
    server.sendHeader("Location", String("/configuration/"), true);
    server.send(302, "text/plain", "");
}

void handleGetConfiguration()
{
    // Keep some room for the injected values.
    char tmp[sizeof(INDEX) + 256];
    snprintf(tmp, sizeof(tmp), INDEX, config.name, config.ssid, MAX_LEDS, config.num_leds);
    server.send(200, "text/html", tmp);
}

void handleSetConfiguration()
{
    if (!server.hasArg("plain"))
    {
        server.send(400, "text/plain", "Missing message body.\n");
        return;
    }

    const String contentType = server.header("content-type");

    if (contentType != "application/x-www-form-urlencoded")
    {
        char tmp[128];
        snprintf(tmp, 128, "Expecting 'application/x-www-form-urlencoded' content-type, got: '%s'.\n", contentType.c_str());
        server.send(400, "text/plain", tmp);
        return;
    }

    const String name = server.arg("name");
    const String ssid = server.arg("ssid");
    const String passphrase = server.arg("passphrase");
    const uint16_t num_leds = atoi(server.arg("num_leds").c_str());

    if (name.length() >= sizeof(config.name))
    {
        server.send(400, "text/plain", "Name is too big.\n");
        return;
    }

    if (ssid.length() >= sizeof(config.ssid))
    {
        server.send(400, "text/plain", "SSID is too big.\n");
        return;
    }

    if (passphrase.length() >= sizeof(config.passphrase))
    {
        server.send(400, "text/plain", "Passphrase is too big.\n");
        return;
    }

    if (num_leds < 1 || num_leds > MAX_LEDS)
    {
        server.send(400, "text/plain", "Invalid number of LEDs.\n");
        return;
    }

    snprintf(config.name, sizeof(config.name), name.c_str());
    snprintf(config.ssid, sizeof(config.ssid), ssid.c_str());
    snprintf(config.passphrase, sizeof(config.passphrase), passphrase.c_str());
    config.num_leds = num_leds;

    if (!config.Save()) {
        server.send(500, "text/plain", "Failed to save configuration.\n");
        return;
    }

    handleGetIndex();

    delay(1000);
    ESP.restart();
}

void handleGetInfo()
{
    StaticJsonDocument<256> json;

    json["name"] = config.name;
    json["num-leds"] = config.num_leds;
    json["fps"] = config.fps;

    String body;
    serializeJsonPretty(json, body);
    body += '\n';

    server.send(200, "application/json", body);
}

void handleGetStateWithStatusCode(int statusCode)
{
    StaticJsonDocument<256> json;
    state.toJsonDocument(json);

    String body;
    serializeJsonPretty(json, body);
    body += '\n';

    server.send(statusCode, "application/json", body);
}

void handleGetState()
{
    handleGetStateWithStatusCode(200);
}

void handleSetState()
{
    if (!server.hasArg("plain"))
    {
        server.send(400, "text/plain", "Missing message body.\n");
        return;
    }

    const String contentType = server.header("content-type");

    if (contentType != "application/json")
    {
        char tmp[128];
        snprintf(tmp, 128, "Expecting 'application/json' content-type, got: '%s'.\n", contentType.c_str());
        server.send(400, "text/plain", tmp);
        return;
    }

    StaticJsonDocument<256> doc;

    DeserializationError error = deserializeJson(doc, server.arg("plain"));

    if (error)
    {
        char tmp[128];
        snprintf(tmp, 128, "JSON error: %s\n", error.c_str());
        server.send(400, "text/plain", tmp);
        return;
    }

    const StateUpdateResult result = state.fromJsonDocument(doc);

    switch (result)
    {
        case StateUpdateResult_Success:
            handleGetState();
            break;
        case StateUpdateResult_InvalidInput:
            server.send(400, "text/plain", "Invalid state.\n");
            break;
        case StateUpdateResult_OutdatedInput:
            handleGetStateWithStatusCode(409);
            break;
        default:
            server.send(500, "text/plain", "Internal error.\n");
            break;
    }

}

void handleNotFound()
{
    server.send(404, "text/plain", "Not found.\n");
}

void startWebServer(uint16_t port)
{
    // Web
    server.on("/", HTTP_GET, handleGetIndex);
    server.on("/configuration/", HTTP_GET, handleGetConfiguration);
    server.on("/configuration/", HTTP_POST, handleSetConfiguration);
    server.on("/configuration/", HTTP_PUT, handleSetConfiguration);

    // API
    server.on("/v1/info/", HTTP_GET, handleGetInfo);
    server.on("/v1/state/", HTTP_GET, handleGetState);
    server.on("/v1/state/", HTTP_PUT, handleSetState);
    server.onNotFound(handleNotFound);

    const char *headerkeys[] = {"content-type"};
    server.collectHeaders(headerkeys, sizeof(headerkeys) / sizeof(char *));
    server.begin(port);
}

void webServerLoop()
{
    server.handleClient();
}