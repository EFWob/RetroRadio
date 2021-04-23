# Integrate RetroRadio into ESP32-Radio
## Necessary steps
- Include "ARetroRadioExtension.ino" and "ARetroRadioExtension.h" from this repository to your project (src-)directory.
- In the header file, _RETRORADIO_ is defined. This define can be used as guard for conditional compile. If this define is commented out, no
  additional functionality will be included.
- Add _#include "ARetroRadioExtension.h"_ at an appropriate location of _main.cpp_ (for older versions into _ESP32Radio.ino_).
- in _setup()_, add
	- _setupRR ( SETUP_START );_ after NVS has started but before _readprogbuttons()_ is called. Direct before is fine.
	- if you want ETHERNET, add _setupRR ( SETUP_NET );_ after the line _WiFi.disconnect()_ before the code to setup WiFi. Check
	  the flag _NetworkFound_ and only execute the command block starting with _WiFi.mode ( WIFI_STA ) ;_ up to the line _NetworkFound = connectwifi() ;_
	  only if this flag is false after calling _setupRR ( SETUP_NET );_
```
  WiFi.disconnect() ;                                    // After restart router could still
#if defined(RETRORADIO)
  setupRR ( SETUP_NET );
  if (!NetworkFound) 
#endif

    { 
    WiFi.mode ( WIFI_STA ) ;                               // This ESP is a station
    delay ( 500 ) ;                                        // ??
    WiFi.persistent ( false ) ;                            // Do not save SSID and password
    listNetworks() ;                                       // Find WiFi networks
    tcpip_adapter_set_hostname ( TCPIP_ADAPTER_IF_STA,
                               NAME ) ;
    p = dbgprint ( "Connect to WiFi" ) ;                   // Show progress
    tftlog ( p ) ;                                         // On TFT too
    NetworkFound = connectwifi() ;                         // Connect to WiFi network
    }

  dbgprint ( "Start server for commands" ) ;
```
	- _setupRR ( SETUP_DONE );_ near the end of the _setup()_-function, but before "Playtask" and "Spftask" are created (_xTaskCreatePinnedToCore ( 
	   playtask ,...)_
- in _loop()_, add _loopRR()_ at any location.
- in _const char* analyzeCmd ( const char* par, const char* val)_:
	- add _if ( analyzeCmdRR ( reply, argument, value)) return reply;_ before the line _ivalue = value.toInt() ;_
	- at the end, add _analyzeCmdDoneRR ( reply, argument, value );_ just before _return reply ;_

## Strongly suggested steps
Increase the _#defines_ for _DEBUG_BUFFER_SIZE_ and _NVSBUFSIZE_. They are defined in _ARetroRadioExtension.h_ but will most likely be overwritten as the
  _#define_ for them is after the 'natural' position for including the header file. Suggestion is to use compile guard as such:
```
#if !defined(RETRORADIO)
// Debug buffer size
#define DEBUG_BUFFER_SIZE 150
#define NVSBUFSIZE 150
#endif
```
## Nice to have steps

