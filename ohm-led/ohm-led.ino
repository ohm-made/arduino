#include "config.h"
#include "reset.h"
#include "state.h"
#include "web.h"

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>

ESP8266WiFiMulti wifiMulti;

void setup(void)
{
  Serial.begin(74880);
  delay(1000);

  Serial.println();
  Serial.println(F("Initializing..."));

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(EXTERNAL_LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);

  digitalWrite(EXTERNAL_LED_PIN, HIGH);

  if (!config.Load())
  {
    Serial.println(F("No existing configuration was found. Assuming default configuration."));
  }
  else
  {
    Serial.println(F("Loaded existing configuration."));
  }

  setupState();
  Serial.printf("Controller has %d led(s).\n", config.num_leds);

  Serial.println(F("Initializing WiFi..."));

  if (!config.hasSSID())
  {
    Serial.println(F("WiFi has never been configured. Starting in Access-Point mode..."));

    IPAddress ip(192, 168, 16, 1);
    IPAddress gateway(192, 168, 16, 1);
    IPAddress subnet(255, 255, 255, 0);

    if (!WiFi.softAPConfig(ip, gateway, subnet))
    {
      Serial.println(F("Failed to configure WiFi access-point."));
      return;
    }

    // Uncommenting this prevents the DHCP server to send a default gateway.
    //uint8_t mode = 0;
    //wifi_softap_set_dhcps_offer_option(OFFER_ROUTER, &mode);

    if (WiFi.softAP(Config::AP_SSID, Config::AP_PASSPHRASE))
    {
      Serial.printf("Access-Point SSID: %s\n", Config::AP_SSID);
      Serial.printf("Access-Point passphrase: %s\n", Config::AP_PASSPHRASE);
      Serial.printf("Access-Point IP address: %s\n", WiFi.softAPIP().toString().c_str());
    }
    else
    {
      Serial.println(F("Failed to start Access-Point! Something might be wrong with the chip."));
    }
  }
  else
  {
    Serial.println(F("WiFi has been configured. Starting in client mode..."));

    WiFi.softAPdisconnect(true);
    wifiMulti.addAP(config.ssid, config.passphrase);
    Serial.printf("Connecting to '%s'...\n", config.ssid);

    while (wifiMulti.run() != WL_CONNECTED)
    {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(125);
      digitalWrite(LED_BUILTIN, LOW);
      delay(125);
    }

    Serial.printf("Connected to '%s'.\n", config.ssid);
    Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
  }

  startWebServer(config.http_port);

  Serial.printf("HTTP server started on port %d.\n", config.http_port);

  if (config.hasName())
  {
    Serial.printf("Using configured name '%s' as a hostname.\n", config.name);
    WiFi.hostname(config.name);

    if (MDNS.begin(config.name, WiFi.localIP()))
    {
      Serial.println(F("MDNS has been set up."));
      MDNS.addService("http", "tcp", config.http_port);
      MDNS.addService("ohm-led", "tcp", config.http_port);
    }
    else
    {
      Serial.println(F("Failed to setup MDNS."));
    }
  }

  digitalWrite(EXTERNAL_LED_PIN, LOW);
}

void loop(void)
{
  resetLoop();
  stateLoop();
  webServerLoop();
  MDNS.update();
}
