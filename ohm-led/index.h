#pragma once

constexpr char INDEX[] = R"=====(
<!DOCTYPE html>
<html lang="en">
	<head>
			<title>Ohm-Led configuration</title>
            <style type="text/css">
                html {
                    font-family: sans-serif;
                }

                form {
                    width: 100;
                }

                form > div {
                    margin: 1em 0;
                }

                form > div > input {
                    width: 100;
                    border: 1px solid silver;
                }
            </style>
	</head>
    <body>
        <h1>Ohm-Led configuration</h1>
        <p>Use this page to configure your Ohm-Led device.</p>
        <form action="/configuration/" method="post">
            <div>
                <label for="name">Name: </label>
                <input type="text" name="name" id="name" value="%s" required>
            </div>
            <div>
                <label for="ssid">SSID: </label>
                <input type="text" name="ssid" id="ssid" value="%s" required>
            </div>
            <div>
                <label for="passphrase">Passphrase: </label>
                <input type="password" name="passphrase" id="passphrase" required>
            </div>
            <div>
                <label for="num_leds">Number of LEDs: </label>
                <input type="number" name="num_leds" id="num_leds" min="1" max="%d" value="%d" required>
            </div>
            <div>
                <input type="submit" value="Apply configuration">
            </div>
        </form>
    </body>
</html>
)=====";