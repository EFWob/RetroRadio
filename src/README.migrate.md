# Integrate RetroRadio into ESP32-Radio

__TODO: this does not work that way for now. There is still some data (global variables) that need to be shifted from main RetroRadio.ino to
addRR.ino__


## Necessary steps
- Include "addRR.ino" and "addRR.h" from this repository to your project (src-)directory.
- In the header file, _RETRORADIO_ is defined. This define can be used as guard for conditional compile. If this define is commented out, no
  additional functionality will be included.
- Add _#include "addRR.h"_ at an appropriate location of _main.cpp_ (for older versions into _ESP32Radio.ino_).
- in _setup()_, add _setupRR ( SETUP_START );_ after NVS has started but before _readprogbuttons()_ is called. Direct before is fine.
- this step also starts Ethernet, if that interface is existing and the configuration is correct. If Ethernet is startet succesfully , you must check
	  the flag _NetworkFound_ and only execute the command block starting with _WiFi.mode ( WIFI_STA ) ;_ up to the line _NetworkFound = connectwifi() ;_
	  only if this flag is false. 
```
  WiFi.disconnect() ;                                    // After restart router could still
#if defined(RETRORADIO)
  if ( false == NetworkFound ) 	
    { 
#endif
    WiFi.mode ( WIFI_STA ) ;                               // This ESP is a station
    delay ( 500 ) ;                                        // ??
    WiFi.persistent ( false ) ;                            // Do not save SSID and password
    listNetworks() ;                                       // Find WiFi networks
    tcpip_adapter_set_hostname ( TCPIP_ADAPTER_IF_STA,
                               NAME ) ;
    p = dbgprint ( "Connect to WiFi" ) ;                   // Show progress
    tftlog ( p ) ;                                         // On TFT too
    NetworkFound = connectwifi() ;                         // Connect to WiFi network
#if defined(RETRORADIO)
	}
#endif
  dbgprint ( "Start server for commands" ) ;
```
- in _setup()_, add _setupRR ( SETUP_DONE );_ near the end of the _setup()_-function, best before "Playtask" and "Spftask" are created (_xTaskCreatePinnedToCore ( 
	   playtask ,...)_
- in _loop()_, add _loopRR()_ at any location.
- in _const char* analyzeCmd ( const char* str ) ;_, add _return analyzeCmdsRR ( str ) ;_ as very first statement to the function body.

## Strongly suggested steps
Increase the _#defines_ for _DEBUG_BUFFER_SIZE_ and _NVSBUFSIZE_. They are defined in _addRR.h_ but will most likely be overwritten as the
  _#define_ for them is after the 'natural' position for including the header file. Suggestion is to use compile guard as such:
```
#if !defined(RETRORADIO)
// Debug buffer size
#define DEBUG_BUFFER_SIZE 150
#define NVSBUFSIZE 150
#endif
```
## Nice to have steps
- some functions are no longer needed, could be deleted. List/description to follow.
