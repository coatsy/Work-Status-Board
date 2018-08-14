# Working status board
A project to display my status on my door so my family know whether they can disturb me or not - initially a present from my daughter.

## Setup
* Clone or download this repo
* Copy `secrets.h.template` to `secrets.h` and fill in your SSID, WPA Key and an Auth Header secret. Notice that `secrets.h` is excluded from source control via `.gitignore` so you don't accidentally check it in
* Open the sketch in the Arduino IDE
* Update `TestESP8266.ino` so your pis are correct
* Set the board to whichever WEMOS board you have. I've tested this on the original D1 R1 and on the D1 Mini
* Wire up the board
* Open the serial monitor and set it to 115200bps (unless you changed the constant in the sketch)
* Upload the sketch

The LEDs should blash on and off together, then they should cycle until a WiFi connection is established.

Once the board is online, all the LEDS should go off and the SSID and the IP Address will be displayed in the serial window.

Using a web browser on the same network, navigate to `http://<ipaddress>` to get your status.

To set status, you need to pass a header called `AuthHeader` with the value you put in `secrets.h`. If you don't pass the header, or the value is wrong, you'll get a `404` when you try setting the lights.

Setting a light is a matter of `GET`ting` one of the following:
* `/onACall`
* `/coding`
* `/workingHard`
* `/hardlyWorking`
* `/allOff`

Each of these, if the Auth Header matches, will set the appropriate LED and redirect to `/` to display the status

