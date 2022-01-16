Simple client to send ESP-Now messages to the RetroRadio. Can be used as starting point for further ideas.
Only thing that needs to be changed is the _#define_ for the WiFi channel of the SSID the radio is connected to (for ESP-Now both sides need to be on the same WiFi-Channel).

The handling is pretty simple:
  - if you enter anything on the Serial input, that will be send as ESP-Now (broadcast) with the addressee set to "RADIO"
  - only if the input starts with the dash-sign '-' (leading whitespaces are ignored) the word following the dash is used as specific radio name the content following the name is send to (i. e. the input _"-testradio volume=80"_) will send the command _volume=80_ to the addressee "testradio"


