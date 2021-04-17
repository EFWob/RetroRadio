#define ETHERNET 2  // Set to '0' if you do not want Ethernet support at all
                    // Set to 1 to compile with Ethernet support
                    // Set to 2 to compile with Ethernet depending on board setting
                    //      (works for Olimex POE and most likely Olimex POE ISO)
#define RETRORADIO

//***************************************************************************************************
//*  ESP32_Radio -- Webradio receiver for ESP32, VS1053 MP3 module and optional display.            *
//*                 By Ed Smallenburg.                                                              *
//***************************************************************************************************
// ESP32 libraries used:
//  - WiFiMulti
//  - nvs
//  - Adafruit_ST7735
//  - ArduinoOTA
//  - PubSubClient
//  - SD
//  - FS
//  - update
// A library for the VS1053 (for ESP32) is not available (or not easy to find).  Therefore
// a class for this module is derived from the maniacbug library and integrated in this sketch.
//
// See http://www.internet-radio.com for suitable stations.  Add the stations of your choice
// to the preferences in either Esp32_radio_init.ino sketch or through the webinterface.
//
// Brief description of the program:
// First a suitable WiFi network is found and a connection is made.
// Then a connection will be made to a shoutcast server.  The server starts with some
// info in the header in readable ascii, ending with a double CRLF, like:
//  icy-name:Classic Rock Florida - SHE Radio
//  icy-genre:Classic Rock 60s 70s 80s Oldies Miami South Florida
//  icy-url:http://www.ClassicRockFLorida.com
//  content-type:audio/mpeg
//  icy-pub:1
//  icy-metaint:32768          - Metadata after 32768 bytes of MP3-data
//  icy-br:128                 - in kb/sec (for Ogg this is like "icy-br=Quality 2"
//
// After de double CRLF is received, the server starts sending mp3- or Ogg-data.  For mp3, this
// data may contain metadata (non mp3) after every "metaint" mp3 bytes.
// The metadata is empty in most cases, but if any is available the content will be
// presented on the TFT.
// Pushing an input button causes the player to execute a programmable command.
//
// The display used is a Chinese 1.8 color TFT module 128 x 160 pixels.
// Now there is room for 26 characters per line and 16 lines.
// Software will work without installing the display.
// Other displays are also supported. See documentation.
// The SD card interface of the module may be used to play mp3-tracks on the SD card.
//
// For configuration of the WiFi network(s): see the global data section further on.
//
// The VSPI interface is used for VS1053, TFT and SD.
//
// Wiring. Note that this is just an example.  Pins (except 18,19 and 23 of the SPI interface)
// can be configured in the config page of the web interface.
// ESP32dev Signal  Wired to LCD        Wired to VS1053      SDCARD   Wired to the rest
// -------- ------  --------------      -------------------  ------   ---------------
// GPIO32           -                   pin 1 XDCS            -       -
// GPIO5            -                   pin 2 XCS             -       -
// GPIO4            -                   pin 4 DREQ            -       -
// GPIO2            pin 3 D/C or A0     -                     -       -
// GPIO22           -                   -                     CS      -
// GPIO16   RXD2    -                   -                     -       TX of NEXTION (if in use)
// GPIO17   TXD2    -                   -                     -       RX of NEXTION (if in use)
// GPIO18   SCK     pin 5 CLK or SCK    pin 5 SCK             CLK     -
// GPIO19   MISO    -                   pin 7 MISO            MISO    -
// GPIO23   MOSI    pin 4 DIN or SDA    pin 6 MOSI            MOSI    -
// GPIO15           pin 2 CS            -                     -       -
// GPI03    RXD0    -                   -                     -       Reserved serial input
// GPIO1    TXD0    -                   -                     -       Reserved serial output
// GPIO34   -       -                   -                     -       Optional pull-up resistor
// GPIO35   -       -                   -                     -       Infrared receiver VS1838B
// GPIO25   -       -                   -                     -       Rotary encoder CLK
// GPIO26   -       -                   -                     -       Rotary encoder DT
// GPIO27   -       -                   -                     -       Rotary encoder SW
// -------  ------  ---------------     -------------------  ------   ----------------
// GND      -       pin 8 GND           pin 8 GND                     Power supply GND
// VCC 5 V  -       pin 7 BL            -                             Power supply
// VCC 5 V  -       pin 6 VCC           pin 9 5V                      Power supply
// EN       -       pin 1 RST           pin 3 XRST                    -
//
// 26-04-2017, ES: First set-up, derived from ESP8266 version.
// 08-05-2017, ES: Handling of preferences.
// 20-05-2017, ES: Handling input buttons and MQTT.
// 22-05-2017, ES: Save preset, volume and tone settings.
// 23-05-2017, ES: No more calls of non-iram functions on interrupts.
// 24-05-2017, ES: Support for featherboard.
// 26-05-2017, ES: Correction playing from .m3u playlist. Allow single hidden SSID.
// 30-05-2017, ES: Add SD card support (FAT format), volume indicator.
// 26-06-2017, ES: Correction: start in AP-mode if no WiFi networks configured.
// 28-06-2017, ES: Added IR interface.
// 30-06-2017, ES: Improved functions for SD card play.
// 03-07-2017, ES: Webinterface control page shows current settings.
// 04-07-2017, ES: Correction MQTT subscription. Keep playing during long operations.
// 08-07-2017, ES: More space for streamtitle on TFT.
// 18-07-2017, ES: Time Of Day on TFT.
// 19-07-2017, ES: Minor corrections.
// 26-07-2017, ES: Flexible pin assignment. Add rotary encoder switch.
// 27-07-2017, ES: Removed tinyXML library.
// 18-08-2017, Es: Minor corrections
// 28-08-2017, ES: Preferences for pins used for SPI bus,
//                 Corrected bug in handling programmable pins,
//                 Introduced touch pins.
// 30-08-2017, ES: Limit number of retries for MQTT connection.
//                 Added MDNS responder.
// 11-11-2017, ES: Increased ringbuffer.  Measure bit rate.
// 13-11-2017, ES: Forward declarations.
// 16-11-2017, ES: Replaced ringbuffer by FreeRTOS queue, play function on second CPU,
//                 Included improved rotary switch routines supplied by fenyvesi,
//                 Better IR sensitivity.
// 30-11-2017, ES: Hide passwords in config page.
// 01-12-2017, ES: Better handling of playlist.
// 07-12-2017, ES: Faster handling of config screen.
// 08-12-2017, ES: More MQTT items to publish, added pin_shutdown.
// 13-12-2017, ES: Correction clear LCD.
// 15-12-2017, ES: Correction defaultprefs.h.
// 18-12-2017, ES: Stop playing during config.
// 02-01-2018, ES: Stop/resume is same command.
// 22-01-2018, ES: Read ADC (GPIO36) and display as a battery capacity percentage.
// 13-02-2018, ES: Stop timer during NVS write.
// 15-02-2018, ES: Correction writing wifi credentials in NVS.
// 03-03-2018, ES: Correction bug IR pinnumber.
// 05-03-2018, ES: Improved rotary encoder interface.
// 10-03-2018, ES: Minor corrections.
// 13-04-2018, ES: Guard against empty string send to TFT, thanks to Andreas Spiess.
// 16-04-2018, ES: ID3 tags handling while playing from SD.
// 25-04-2018, ES: Choice of several display boards.
// 30-04-2018, ES: Bugfix: crash when no IR is configured, no display without VS1063.
// 08-05-2018, ES: 1602 LCD display support (limited).
// 11-05-2018, ES: Bugfix: incidental crash in isr_enc_turn().
// 30-05-2018, ES: Bugfix: Assigned DRAM to global variables used in timer ISR.
// 31-05-2018, ES: Bugfix: Crashed if I2C is used, but pins not defined.
// 01-06-2018, ES: Run Playtask on CPU 0.
// 04-06-2018, ES: Made handling of playlistdata more tolerant (NDR).
// 09-06-2018, ES: Typo in defaultprefs.h.
// 10-06-2018, ES: Rotary encoder, interrupts on all 3 signals.
// 25-06-2018, ES: Timing of mp3loop.  Limit read from stream to free queue space.
// 16-07-2018, ES: Correction tftset().
// 25-07-2018, ES: Correction touch pins.
// 30-07-2018, ES: Added GPIO39 and inversed shutdown pin.  Thanks to fletsche.
// 31-07-2018, ES: Added TFT backlight control.
// 01-08-2018, ES: Debug info for IR.  Shutdown amplifier if volume is 0.
// 02-08-2018, ES: Added support for ILI9341 display.
// 03-08-2018, ES: Added playlistposition for MQTT.
// 06-08-2018, ES: Correction negative time offset, OTA through remote host.
// 16-08-2018, ES: Added Nextion support.
// 18-09-2018, ES: "uppreset" and "downpreset" for MP3 player.
// 04-10-2018, ES: Fixed compile error OLED 64x128 display.
// 09-10-2018, ES: Bug fix xSemaphoreTake.
// 05-01-2019, ES: Fine tune datarate.
// 05-01-2019, ES: Basic http authentication. (just one user)
// 11-02-2019, ES: MQTT topic and subtopic size enlarged.
// 24-04-2019, ES: Do not lock SPI during gettime().  Calling gettime may take a long time.
// 15-05-2019, ES: MAX number of presets as a defined constant.
// 16-12-2019, ES: Modify of claimSPI() function for debugability.
// 21-12-2019, ES: Check chip version.
// 23-03-2020, ES: Allow playlists on SD card.
// 25-03-2020, ES: End of playlist: start over.
// 09-07-2020, ES: Add CH376 support.
// 14-07-2020, ES: Dynamic status display in webinterface.
// 17-09-2020, ES: Support for LCD2004. Thanks to mrohner.
// 30-09-2020, ES: Ready for ch376msc library Version 1.4.4
// 14-10-2020, ES: Clear artist and title on new station connect
// 19-10-2020, ES: Fixed LCD2004 errror
//
//
// Define the version number, also used for webserver as Last-Modified header and to
// check version for update.  The format must be exactly as specified by the HTTP standard!
#define VERSION     "Mon, 19 Oct 2020 14:12:00 GMT"
// ESP32-Radio can be updated (OTA) to the latest version from a remote server.
// The download uses the following server and files:
#define UPDATEHOST  "smallenburg.nl"                    // Host for software updates
#define BINFILE     "/Arduino/Esp32_radio.ino.bin"      // Binary file name for update software
#define TFTFILE     "/Arduino/ESP32-Radio.tft"          // Binary file name for update NEXTION image
//
// Define type of local filesystem(s).  See documentation.
//#define CH376                          // For CXH376 support (reading files from USB stick)
#define SDCARD                         // For SD card support (reading files from SD card)
// Define (just one) type of display.  See documentation.
//#define BLUETFT                        // Works also for RED TFT 128x160
//#define OLED                         // 64x128 I2C OLED
#define DUMMYTFT                     // Dummy display
//#define LCD1602I2C                   // LCD 1602 display with I2C backpack
//#define LCD2004I2C                   // LCD 2004 display with I2C backpack
//#define ILI9341                      // ILI9341 240*320
//#define NEXTION                      // Nextion display. Uses UART 2 (pin 16 and 17)
//

#include <nvs.h>
#include <PubSubClient.h>
#include <WiFiMulti.h>
#include <ESPmDNS.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <SPI.h>
#include <ArduinoOTA.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <esp_task_wdt.h>
#include <esp_partition.h>
#include <driver/adc.h>
#include <Update.h>
#include <base64.h>


#if !defined(ETHERNET) || !defined(RETRORADIO)
#define ETHERNET 0
#endif

#if (ETHERNET == 2) && (defined(ARDUINO_ESP32_POE) || defined(ARDUINO_ESP32_POE_ISO))
#define ETHERNET 1
#else 
#define ETHERNET 0
#endif


#if (ETHERNET == 1)
#include <ETH.h>
#define ETHERNET_CONNECT_TIMEOUT      5 // How long to wait for ethernet (at least)?
#endif

// Number of entries in the queue
#define QSIZ 400
// Debug buffer size
#define DEBUG_BUFFER_SIZE 150
#define NVSBUFSIZE 150
// Access point name if connection to WiFi network fails.  Also the hostname for WiFi and OTA.
// Note that the password of an AP must be at least as long as 8 characters.
// Also used for other naming.
#define NAME "NetzRadio"
#if defined(RETRORADIO)
#include "ARetroRadioExtension.h"
#endif

// Max number of presets in preferences
#define MAXPRESETS 200
// Maximum number of MQTT reconnects before give-up
#define MAXMQTTCONNECTS 5
// Adjust size of buffer to the longest expected string for nvsgetstr
#if defined(RETRORADIO)
#define NVSBUFSIZE 250
#else
#define NVSBUFSIZE 150
#endif
// Position (column) of time in topline relative to end
#define TIMEPOS -52
// SPI speed for SD card
#define SDSPEED 1000000
// Size of metaline buffer
#define METASIZ 1024
// Max. number of NVS keys in table
#define MAXKEYS 500
// Time-out [sec] for blanking TFT display (BL pin)
#define BL_TIME 45
//
// Subscription topics for MQTT.  The topic will be pefixed by "PREFIX/", where PREFIX is replaced
// by the the mqttprefix in the preferences.  The next definition will yield the topic
// "ESP32Radio/command" if mqttprefix is "ESP32Radio".
#define MQTT_SUBTOPIC     "command"           // Command to receive from MQTT
//
#define otaclient mp3client                   // OTA uses mp3client for connection to host

//**************************************************************************************************
// Forward declaration and prototypes of various functions.                                        *
//**************************************************************************************************
void        displaytime ( const char* str, uint16_t color = 0xFFFF ) ;
void        showstreamtitle ( const char* ml, bool full = false ) ;
void        handlebyte_ch ( uint8_t b ) ;
void        handleFSf ( const String& pagename ) ;
void        handleCmd()  ;
char*       dbgprint( const char* format, ... ) ;
const char* analyzeCmds ( String s ) ;
const char* analyzeCmd ( const char* str ) ;
const char* analyzeCmd ( const char* par, const char* val ) ;
void        chomp ( String &str ) ;
String      httpheader ( String contentstype ) ;
bool        nvssearch ( const char* key ) ;
void        mp3loop() ;
void        stop_mp3client () ;
void        tftlog ( const char *str ) ;
void        tftset ( uint16_t inx, const char *str ) ;
void        tftset ( uint16_t inx, String& str ) ;
void        playtask ( void * parameter ) ;       // Task to play the stream
void        spftask ( void * parameter ) ;        // Task for special functions
void        gettime() ;
void        reservepin ( int8_t rpinnr ) ;
void        claimSPI ( const char* p ) ;          // Claim SPI bus for exclusive access
void        releaseSPI() ;                        // Release the claim
char        utf8ascii ( char ascii ) ;            // Convert UTF8 char to normal char
void        utf8ascii_ip ( char* s ) ;            // In place conversion full string
String      utf8ascii ( const char* s ) ;
uint32_t    ssconv ( const uint8_t* bytes ) ;


//**************************************************************************************************
// Several structs and enums.                                                                      *
//**************************************************************************************************
//


enum IR_protocol { IR_NONE = 0, IR_NEC, IR_RC5 } ;      // Known IR-Protocols

enum RC5State 
{
      RC5STATE_START1, 
      RC5STATE_MID1, 
      RC5STATE_MID0, 
      RC5STATE_START0, 
      RC5STATE_ERROR, 
      RC5STATE_BEGIN, 
      RC5STATE_END
} ;

struct IR_data 
{
   uint16_t command ;                                 // Command received
   uint16_t exitcommand ;                             // Last command, key released, release event due
   uint32_t exittimeout ;                             // Start for exittimeout
   uint16_t repeat_counter ;                          // Repeat counter
   IR_protocol protocol ;                             // Protocol of command (set to IR_NONE to report to ISR as consumed)
} ;

enum fs_type { FS_USB, FS_SD } ;                      // USB- or SD interface

struct scrseg_struct                                  // For screen segments
{
  bool     update_req ;                               // Request update of screen
  uint16_t color ;                                    // Textcolor
  uint16_t y ;                                        // Begin of segment row
  uint16_t height ;                                   // Height of segment
  String   str ;                                      // String to be displayed
} ;

enum qdata_type { QDATA, QSTARTSONG, QSTOPSONG } ;    // datatyp in qdata_struct
struct qdata_struct
{
  int datatyp ;                                       // Identifier
  __attribute__((aligned(4))) uint8_t buf[32] ;       // Buffer for chunk
} ;

struct ini_struct
{
  String         mqttbroker ;                         // The name of the MQTT broker server
  String         mqttprefix ;                         // Prefix to use for topics
  uint16_t       mqttport ;                           // Port, default 1883
  String         mqttuser ;                           // User for MQTT authentication
  String         mqttpasswd ;                         // Password for MQTT authentication
  uint8_t        reqvol ;                             // Requested volume
  uint8_t        rtone[4] ;                           // Requested bass/treble settings
  int16_t        newpreset ;                          // Requested preset
  String         clk_server ;                         // Server to be used for time of day clock
  int8_t         clk_offset ;                         // Offset in hours with respect to UTC
  int8_t         clk_dst ;                            // Number of hours shift during DST
  int            ir_pin ;                             // GPIO connected to output of IR decoder (both RC5 & NEC protocol)
  int            ir_pin_nec ;                         // GPIO for IR decoder if explicitely NEC protocol only is desired
  int            ir_pin_rc5 ;                         // GPIO for IR decoder if explicitely RC5 protocol only is desired  
  int            enc_clk_pin ;                        // GPIO connected to CLK of rotary encoder
  int            enc_dt_pin ;                         // GPIO connected to DT of rotary encoder
  int            enc_sw_pin ;                         // GPIO connected to SW of rotary encoder
  int            tft_cs_pin ;                         // GPIO connected to CS of TFT screen
  int            tft_dc_pin ;                         // GPIO connected to D/C or A0 of TFT screen
  int            tft_scl_pin ;                        // GPIO connected to SCL of i2c TFT screen
  int            tft_sda_pin ;                        // GPIO connected to SDA of I2C TFT screen
  int            tft_bl_pin ;                         // GPIO to activate BL of display
  int            tft_blx_pin ;                        // GPIO to activate BL of display (inversed logic)
  int            sd_cs_pin ;                          // GPIO connected to CS of SD card
  int            vs_cs_pin ;                          // GPIO connected to CS of VS1053
  int            vs_dcs_pin ;                         // GPIO connected to DCS of VS1053
  int            vs_dreq_pin ;                        // GPIO connected to DREQ of VS1053
  int            vs_shutdown_pin ;                    // GPIO to shut down the amplifier
  int            vs_shutdownx_pin ;                   // GPIO to shut down the amplifier (inversed logic)
  int            spi_sck_pin ;                        // GPIO connected to SPI SCK pin
  int            spi_miso_pin ;                       // GPIO connected to SPI MISO pin
  int            spi_mosi_pin ;                       // GPIO connected to SPI MOSI pin
  int            ch376_cs_pin ;                       // GPIO connected to CH376 SS
  int            ch376_int_pin ;                      // GPIO connected to CH376 INT
#if (ETHERNET == 1)
  int            eth_phy_addr;                        // Ethernet Phy Address setting
  int            eth_phy_power;                       // Ethernet Power setting
  int            eth_phy_mdc;                         // Ethernet MDC setting
  int            eth_phy_mdio;                        // Ethernet Phy MDIO setting
  int            eth_phy_type;                        // Ethernet Phy Type setting
  int            eth_clk_mode;                        // Ethernet clock mode setting
  int            eth_timeout;                         // Ethernet timeout (in seconds)
#endif
#ifdef RETRORADIO
  int            vol_min;                              // minimum volume
  int            vol_max;                              // maximum volume
#endif
  uint16_t       bat0 ;                               // ADC value for 0 percent battery charge
  uint16_t       bat100 ;                             // ADC value for 100 percent battery charge
} ;

struct WifiInfo_t                                     // For list with WiFi info
{
  uint8_t inx ;                                       // Index as in "wifi_00"
  char * ssid ;                                       // SSID for an entry
  char * passphrase ;                                 // Passphrase for an entry
} ;

struct nvs_entry
{
  uint8_t  Ns ;                                       // Namespace ID
  uint8_t  Type ;                                     // Type of value
  uint8_t  Span ;                                     // Number of entries used for this item
  uint8_t  Rvs ;                                      // Reserved, should be 0xFF
  uint32_t CRC ;                                      // CRC
  char     Key[16] ;                                  // Key in Ascii
  uint64_t Data ;                                     // Data in entry
} ;

struct nvs_page                                       // For nvs entries
{ // 1 page is 4096 bytes
  uint32_t  State ;
  uint32_t  Seqnr ;
  uint32_t  Unused[5] ;
  uint32_t  CRC ;
  uint8_t   Bitmap[32] ;
  nvs_entry Entry[126] ;
} ;

struct keyname_t                                      // For keys in NVS
{
  char      Key[16] ;                                 // Max length is 15 plus delimeter
} ;

//**************************************************************************************************
// Global data section.                                                                            *
//**************************************************************************************************
// There is a block ini-data that contains some configuration.  Configuration data is              *
// saved in the preferences by the webinterface.  On restart the new data will                     *
// de read from these preferences.                                                                 *
// Items in ini_block can be changed by commands from webserver/MQTT/Serial.                       *
//**************************************************************************************************

enum display_t { T_UNDEFINED, T_BLUETFT, T_OLED,             // Various types of display
                 T_DUMMYTFT, T_LCD1602I2C, T_LCD2004I2C,
                 T_ILI9341, T_NEXTION } ;

enum datamode_t { INIT = 0x1, HEADER = 0x2, DATA = 0x4,      // State for datastream
                  METADATA = 0x8, PLAYLISTINIT = 0x10,
                  PLAYLISTHEADER = 0x20, PLAYLISTDATA = 0x40,
                  STOPREQD = 0x80, STOPPED = 0x100
                } ;

// Global variables
int               DEBUG = 1 ;                            // Debug on/off
int               numSsid ;                              // Number of available WiFi networks
WiFiMulti         wifiMulti ;                            // Possible WiFi networks
ini_struct        ini_block ;                            // Holds configurable data
WiFiServer        cmdserver ( 80 ) ;                     // Instance of embedded webserver, port 80
WiFiClient        mp3client ;                            // An instance of the mp3 client, also used for OTA
WiFiClient        cmdclient ;                            // An instance of the client for commands
WiFiClient        wmqttclient ;                          // An instance for mqtt
PubSubClient      mqttclient ( wmqttclient ) ;           // Client for MQTT subscriber
HardwareSerial*   nxtserial = NULL ;                     // Serial port for NEXTION (if defined)
TaskHandle_t      maintask ;                             // Taskhandle for main task
TaskHandle_t      xplaytask ;                            // Task handle for playtask
TaskHandle_t      xspftask ;                             // Task handle for special functions
SemaphoreHandle_t SPIsem = NULL ;                        // For exclusive SPI usage
hw_timer_t*       timer = NULL ;                         // For timer
char              timetxt[9] ;                           // Converted timeinfo
char              cmd[130] ;                             // Command from MQTT or Serial
uint8_t           tmpbuff[6000] ;                        // Input buffer for mp3 or data stream 
QueueHandle_t     dataqueue ;                            // Queue for mp3 datastream
QueueHandle_t     spfqueue ;                             // Queue for special functions
qdata_struct      outchunk ;                             // Data to queue
qdata_struct      inchunk ;                              // Data from queue
uint8_t*          outqp = outchunk.buf ;                 // Pointer to buffer in outchunk
uint32_t          totalcount = 0 ;                       // Counter mp3 data
datamode_t        datamode ;                             // State of datastream
int               metacount ;                            // Number of bytes in metadata
int               datacount ;                            // Counter databytes before metadata
char              metalinebf[METASIZ + 1] ;              // Buffer for metaline/ID3 tags
int16_t           metalinebfx ;                          // Index for metalinebf
String            icystreamtitle ;                       // Streamtitle from metadata
String            icyname ;                              // Icecast station name
String            ipaddress ;                            // Own IP-address
int               bitrate ;                              // Bitrate in kb/sec
int               mbitrate ;                             // Measured bitrate
int               metaint = 0 ;                          // Number of databytes between metadata
int16_t           currentpreset = -1 ;                   // Preset station playing
String            host ;                                 // The URL to connect to or file to play
String            playlist ;                             // The URL of the specified playlist
bool              hostreq = false ;                      // Request for new host
bool              reqtone = false ;                      // New tone setting requested
bool              muteflag = false ;                     // Mute output
bool              resetreq = false ;                     // Request to reset the ESP32
bool              updatereq = false ;                    // Request to update software from remote host
bool              NetworkFound = false ;                 // True if WiFi network connected
                                                         // Or Ethernet connected. To distinguish use next flag
#if (ETHERNET == 1)
bool              EthernetFound = false ;                // True if Ethernet connected
#endif
bool              mqtt_on = false ;                      // MQTT in use
String            networks ;                             // Found networks in the surrounding
uint16_t          mqttcount = 0 ;                        // Counter MAXMQTTCONNECTS
int8_t            playingstat = 0 ;                      // 1 if radio is playing (for MQTT)
int16_t           playlist_num = 0 ;                     // Nonzero for selection from playlist
fs_type           usb_sd = FS_USB ;                      // SD or USB interface
uint32_t          mp3filelength ;                        // File length
bool              localfile = false ;                    // Play from local mp3-file or not
bool              chunked = false ;                      // Station provides chunked transfer
int               chunkcount = 0 ;                       // Counter for chunked transfer
String            http_getcmd ;                          // Contents of last GET command
String            http_rqfile ;                          // Requested file
bool              http_response_flag = false ;           // Response required
volatile IR_data  ir = { 0, 0, 0, 0, IR_NONE } ;         // IR data received
//uint32_t          ir_value = 0 ;                         // IR code
//volatile uint32_t ir_repeatcode = 0;                     // Code to report, if repeat shot has been detected
uint32_t          ir_0 = 550 ;                           // Average duration of an IR short pulse
uint32_t          ir_1 = 1650 ;                          // Average duration of an IR long pulse
struct tm         timeinfo ;                             // Will be filled by NTP server
bool              time_req = false ;                     // Set time requested
uint16_t          adcval ;                               // ADC value (battery voltage)
uint32_t          clength ;                              // Content length found in http header
uint32_t          max_mp3loop_time = 0 ;                 // To check max handling time in mp3loop (msec)
uint32_t          max_mp3chunk = 0 ;                     // To check max chunk size requested from stream (bytes)
uint32_t          max_mp3av = 0 ;                        // To check max bytes available on stream (bytes)
int16_t           scanios ;                              // TEST*TEST*TEST
int16_t           scaniocount ;                          // TEST*TEST*TEST
uint16_t          bltimer = 0 ;                          // Backlight time-out counter
display_t         displaytype = T_UNDEFINED ;            // Display type
std::vector<WifiInfo_t> wifilist ;                       // List with wifi_xx info
// nvs stuff
nvs_page                nvsbuf ;                         // Space for 1 page of NVS info
const esp_partition_t*  nvs ;                            // Pointer to partition struct
esp_err_t               nvserr ;                         // Error code from nvs functions
uint32_t                nvshandle = 0 ;                  // Handle for nvs access
uint8_t                 namespace_ID ;                   // Namespace ID found
char                    nvskeys[MAXKEYS][16] ;           // Space for NVS keys
std::vector<keyname_t> keynames ;                        // Keynames in NVS
// Rotary encoder stuff
#define sv DRAM_ATTR static volatile
sv uint16_t       clickcount = 0 ;                       // Incremented per encoder click
sv int16_t        rotationcount = 0 ;                    // Current position of rotary switch
sv uint16_t       enc_inactivity = 0 ;                   // Time inactive
sv bool           singleclick = false ;                  // True if single click detected
sv bool           doubleclick = false ;                  // True if double click detected
sv bool           tripleclick = false ;                  // True if triple click detected
sv bool           longclick = false ;                    // True if longclick detected
enum enc_menu_t { VOLUME, PRESET, TRACK } ;              // State for rotary encoder menu
enc_menu_t        enc_menu_mode = VOLUME ;               // Default is VOLUME mode

//
struct progpin_struct                                    // For programmable input pins
{
  int8_t         gpio ;                                  // Pin number
  bool           reserved ;                              // Reserved for connected devices
  bool           avail ;                                 // Pin is available for a command
  String         command ;                               // Command to execute when activated
                                                         // Example: "uppreset=1"
  bool           cur ;                                   // Current state, true = HIGH, false = LOW
} ;

progpin_struct   progpin[] =                             // Input pins and programmed function
{
  {  0, false, false,  "", false },
  //{  1, true,  false,  "", false },                    // Reserved for TX Serial output
  {  2, false, false,  "", false },
  //{  3, true,  false,  "", false },                    // Reserved for RX Serial input
  {  4, false, false,  "", false },
  {  5, false, false,  "", false },
  //{  6, true,  false,  "", false },                    // Reserved for FLASH SCK
  //{  7, true,  false,  "", false },                    // Reserved for FLASH D0
  //{  8, true,  false,  "", false },                    // Reserved for FLASH D1
  //{  9, true,  false,  "", false },                    // Reserved for FLASH D2
  //{ 10, true,  false,  "", false },                    // Reserved for FLASH D3
  //{ 11, true,  false,  "", false },                    // Reserved for FLASH CMD
  { 12, false, false,  "", false },
  { 13, false, false,  "", false },
  { 14, false, false,  "", false },
  { 15, false, false,  "", false },
  { 16, false, false,  "", false },                      // May be UART 2 RX for Nextion
  { 17, false, false,  "", false },                      // May be UART 2 TX for Nextion
  { 18, false, false,  "", false },                      // Default for SPI CLK
  { 19, false, false,  "", false },                      // Default for SPI MISO
  //{ 20, true,  false,  "", false },                    // Not exposed on DEV board
  { 21, false, false,  "", false },                      // Also Wire SDA
  { 22, false, false,  "", false },                      // Also Wire SCL
  { 23, false, false,  "", false },                      // Default for SPI MOSI
  //{ 24, true,  false,  "", false },                    // Not exposed on DEV board
  { 25, false, false,  "", false },
  { 26, false, false,  "", false },
  { 27, false, false,  "", false },
  //{ 28, true,  false,  "", false },                    // Not exposed on DEV board
  //{ 29, true,  false,  "", false },                    // Not exposed on DEV board
  //{ 30, true,  false,  "", false },                    // Not exposed on DEV board
  //{ 31, true,  false,  "", false },                    // Not exposed on DEV board
  { 32, false, false,  "", false },
  { 33, false, false,  "", false },
  { 34, false, false,  "", false },                      // Note, no internal pull-up
  { 35, false, false,  "", false },                      // Note, no internal pull-up
  //{ 36, true,  false,  "", false },                    // Reserved for ADC battery level
  { 39, false,  false,  "", false },                     // Note, no internal pull-up
  { -1, false, false,  "", false }                       // End of list
} ;

struct touchpin_struct                                   // For programmable input pins
{
  int8_t         gpio ;                                  // Pin number GPIO
  bool           reserved ;                              // Reserved for connected devices
  bool           avail ;                                 // Pin is available for a command
  String         command ;                               // Command to execute when activated
  // Example: "uppreset=1"
  bool           cur ;                                   // Current state, true = HIGH, false = LOW
  int16_t        count ;                                 // Counter number of times low level
} ;
touchpin_struct   touchpin[] =                           // Touch pins and programmed function
{
  {   4, false, false, "", false, 0 },                   // TOUCH0
  {   0, true,  false, "", false, 0 },                   // TOUCH1, reserved for BOOT button
  {   2, false, false, "", false, 0 },                   // TOUCH2
  {  15, false, false, "", false, 0 },                   // TOUCH3
  {  13, false, false, "", false, 0 },                   // TOUCH4
  {  12, false, false, "", false, 0 },                   // TOUCH5
  {  14, false, false, "", false, 0 },                   // TOUCH6
  {  27, false, false, "", false, 0 },                   // TOUCH7
  {  33, false, false, "", false, 0 },                   // TOUCH8
  {  32, false, false, "", false, 0 },                   // TOUCH9
  {  -1, false, false, "", false, 0 }                    // End of list
  // End of table
} ;


//**************************************************************************************************
// Pages, CSS and data for the webinterface.                                                       *
//**************************************************************************************************
#include "about_html.h"
#include "config_html.h"
#include "index_html.h"
#include "mp3play_html.h"
#include "radio_css.h"
#include "favicon_ico.h"
#include "defaultprefs.h"

//**************************************************************************************************
// End of global data section.                                                                     *
//**************************************************************************************************




//**************************************************************************************************
//                                     M Q T T P U B _ C L A S S                                   *
//**************************************************************************************************
// ID's for the items to publish to MQTT.  Is index in amqttpub[]
enum { MQTT_IP,     MQTT_ICYNAME, MQTT_STREAMTITLE, MQTT_NOWPLAYING,
       MQTT_PRESET, MQTT_VOLUME, MQTT_PLAYING, MQTT_PLAYLISTPOS
     } ;
enum { MQSTRING, MQINT8, MQINT16 } ;                     // Type of variable to publish

class mqttpubc                                           // For MQTT publishing
{
    struct mqttpub_struct
    {
      const char*    topic ;                             // Topic as partial string (without prefix)
      uint8_t        type ;                              // Type of payload
      void*          payload ;                           // Payload for this topic
      bool           topictrigger ;                      // Set to true to trigger MQTT publish
    } ;
    // Publication topics for MQTT.  The topic will be pefixed by "PREFIX/", where PREFIX is replaced
    // by the the mqttprefix in the preferences.
  protected:
    mqttpub_struct amqttpub[9] =                   // Definitions of various MQTT topic to publish
    { // Index is equal to enum above
      { "ip",              MQSTRING, &ipaddress,        false }, // Definition for MQTT_IP
      { "icy/name",        MQSTRING, &icyname,          false }, // Definition for MQTT_ICYNAME
      { "icy/streamtitle", MQSTRING, &icystreamtitle,   false }, // Definition for MQTT_STREAMTITLE
      { "nowplaying",      MQSTRING, &ipaddress,        false }, // Definition for MQTT_NOWPLAYING
      { "preset" ,         MQINT8,   &currentpreset,    false }, // Definition for MQTT_PRESET
      { "volume" ,         MQINT8,   &ini_block.reqvol, false }, // Definition for MQTT_VOLUME
      { "playing",         MQINT8,   &playingstat,      false }, // Definition for MQTT_PLAYING
      { "playlist/pos",    MQINT16,  &playlist_num,     false }, // Definition for MQTT_PLAYLISTPOS
      { NULL,              0,        NULL,              false }  // End of definitions
    } ;
  public:
    void          trigger ( uint8_t item ) ;                      // Trigger publishig for one item
    void          publishtopic() ;                                // Publish triggerer items
} ;


//**************************************************************************************************
// MQTTPUB  class implementation.                                                                  *
//**************************************************************************************************

//**************************************************************************************************
//                                            T R I G G E R                                        *
//**************************************************************************************************
// Set request for an item to publish to MQTT.                                                     *
//**************************************************************************************************
void mqttpubc::trigger ( uint8_t item )                    // Trigger publishig for one item
{
  amqttpub[item].topictrigger = true ;                     // Request re-publish for an item
}

//**************************************************************************************************
//                                     P U B L I S H T O P I C                                     *
//**************************************************************************************************
// Publish a topic to MQTT broker.                                                                 *
//**************************************************************************************************
void mqttpubc::publishtopic()
{
  int         i = 0 ;                                         // Loop control
  char        topic[80] ;                                     // Topic to send
  const char* payload ;                                       // Points to payload
  char        intvar[10] ;                                    // Space for integer parameter
  while ( amqttpub[i].topic )
  {
    if ( amqttpub[i].topictrigger )                           // Topic ready to send?
    {
      amqttpub[i].topictrigger = false ;                      // Success or not: clear trigger
      sprintf ( topic, "%s/%s", ini_block.mqttprefix.c_str(),
                amqttpub[i].topic ) ;                         // Add prefix to topic
      switch ( amqttpub[i].type )                             // Select conversion method
      {
        case MQSTRING :
          payload = ((String*)amqttpub[i].payload)->c_str() ;
          //payload = pstr->c_str() ;                           // Get pointer to payload
          break ;
        case MQINT8 :
          sprintf ( intvar, "%d",
                    *(int8_t*)amqttpub[i].payload ) ;         // Convert to array of char
          payload = intvar ;                                  // Point to this array
          break ;
        case MQINT16 :
          sprintf ( intvar, "%d",
                    *(int16_t*)amqttpub[i].payload ) ;        // Convert to array of char
          payload = intvar ;                                  // Point to this array
          break ;
        default :
          continue ;                                          // Unknown data type
      }
      dbgprint ( "Publish to topic %s : %s",                  // Show for debug
                 topic, payload ) ;
      if ( !mqttclient.publish ( topic, payload ) )           // Publish!
      {
        dbgprint ( "MQTT publish failed!" ) ;                 // Failed
      }
      return ;                                                // Do the rest later
    }
    i++ ;                                                     // Next entry
  }
}

mqttpubc         mqttpub ;                                    // Instance for mqttpubc


//
//**************************************************************************************************
// VS1053 stuff.  Based on maniacbug library.                                                      *
//**************************************************************************************************
// VS1053 class definition.                                                                        *
//**************************************************************************************************
class VS1053
{
  private:
    int8_t        cs_pin ;                         // Pin where CS line is connected
    int8_t        dcs_pin ;                        // Pin where DCS line is connected
    int8_t        dreq_pin ;                       // Pin where DREQ line is connected
    int8_t        shutdown_pin ;                   // Pin where the shutdown line is connected
    int8_t        shutdownx_pin ;                  // Pin where the shutdown (inversed) line is connected
    uint8_t       curvol ;                         // Current volume setting 0..100%
    const uint8_t vs1053_chunk_size = 32 ;
    // SCI Register
    const uint8_t SCI_MODE          = 0x0 ;
    const uint8_t SCI_STATUS        = 0x1 ;
    const uint8_t SCI_BASS          = 0x2 ;
    const uint8_t SCI_CLOCKF        = 0x3 ;
    const uint8_t SCI_AUDATA        = 0x5 ;
    const uint8_t SCI_WRAM          = 0x6 ;
    const uint8_t SCI_WRAMADDR      = 0x7 ;
    const uint8_t SCI_AIADDR        = 0xA ;
    const uint8_t SCI_VOL           = 0xB ;
    const uint8_t SCI_AICTRL0       = 0xC ;
    const uint8_t SCI_AICTRL1       = 0xD ;
    const uint8_t SCI_num_registers = 0xF ;
    // SCI_MODE bits
    const uint8_t SM_SDINEW         = 11 ;        // Bitnumber in SCI_MODE always on
    const uint8_t SM_RESET          = 2 ;         // Bitnumber in SCI_MODE soft reset
    const uint8_t SM_CANCEL         = 3 ;         // Bitnumber in SCI_MODE cancel song
    const uint8_t SM_TESTS          = 5 ;         // Bitnumber in SCI_MODE for tests
    const uint8_t SM_LINE1          = 14 ;        // Bitnumber in SCI_MODE for Line input
    SPISettings   VS1053_SPI ;                    // SPI settings for this slave
    uint8_t       endFillByte ;                   // Byte to send when stopping song
    bool          okay              = true ;      // VS1053 is working
  protected:
    inline void await_data_request() const
    {
      while ( ( dreq_pin >= 0 ) &&
              ( !digitalRead ( dreq_pin ) ) )
      {
        NOP() ;                                   // Very short delay
      }
    }

    inline void control_mode_on() const
    {
      SPI.beginTransaction ( VS1053_SPI ) ;       // Prevent other SPI users
      digitalWrite ( cs_pin, LOW ) ;
    }

    inline void control_mode_off() const
    {
      digitalWrite ( cs_pin, HIGH ) ;             // End control mode
      SPI.endTransaction() ;                      // Allow other SPI users
    }

    inline void data_mode_on() const
    {
      SPI.beginTransaction ( VS1053_SPI ) ;       // Prevent other SPI users
      //digitalWrite ( cs_pin, HIGH ) ;           // Bring slave in data mode
      digitalWrite ( dcs_pin, LOW ) ;
    }

    inline void data_mode_off() const
    {
      digitalWrite ( dcs_pin, HIGH ) ;            // End data mode
      SPI.endTransaction() ;                      // Allow other SPI users
    }

    uint16_t    read_register ( uint8_t _reg ) const ;
    void        write_register ( uint8_t _reg, uint16_t _value ) const ;
    inline bool sdi_send_buffer ( uint8_t* data, size_t len ) ;
    void        sdi_send_fillers ( size_t length ) ;
    void        wram_write ( uint16_t address, uint16_t data ) ;
    uint16_t    wram_read ( uint16_t address ) ;
    void        output_enable ( bool ena ) ;             // Enable amplifier through shutdown pin(s)

  public:
    // Constructor.  Only sets pin values.  Doesn't touch the chip.  Be sure to call begin()!
    VS1053 ( int8_t _cs_pin, int8_t _dcs_pin, int8_t _dreq_pin,
             int8_t _shutdown_pin, int8_t _shutdownx_pin ) ;
    void     begin() ;                                   // Begin operation.  Sets pins correctly,
    // and prepares SPI bus.
    void     startSong() ;                               // Prepare to start playing. Call this each
    // time a new song starts.
    inline bool playChunk ( uint8_t* data,               // Play a chunk of data.  Copies the data to
                            size_t len ) ;               // the chip.  Blocks until complete.
    // Returns true if more data can be added
    // to fifo
    void     stopSong() ;                                // Finish playing a song. Call this after
    // the last playChunk call.
    void     setVolume ( uint8_t vol ) ;                 // Set the player volume.Level from 0-100,
    // higher is louder.
    void     setTone ( uint8_t* rtone ) ;                // Set the player baas/treble, 4 nibbles for
    // treble gain/freq and bass gain/freq
    inline uint8_t  getVolume() const                    // Get the current volume setting.
    { // higher is louder.
      return curvol ;
    }
    void     printDetails ( const char *header ) ;       // Print config details to serial output
    void     softReset() ;                               // Do a soft reset
    bool     testComm ( const char *header ) ;           // Test communication with module
    inline bool data_request() const
    {
      return ( digitalRead ( dreq_pin ) == HIGH ) ;
    }
    void     AdjustRate ( long ppm2 ) ;                  // Fine tune the datarate

} ;

//**************************************************************************************************
// VS1053 class implementation.                                                                    *
//**************************************************************************************************

VS1053::VS1053 ( int8_t _cs_pin, int8_t _dcs_pin, int8_t _dreq_pin,
                 int8_t _shutdown_pin, int8_t _shutdownx_pin ) :
  cs_pin(_cs_pin), dcs_pin(_dcs_pin), dreq_pin(_dreq_pin), shutdown_pin(_shutdown_pin),
  shutdownx_pin(_shutdownx_pin)
{
}

uint16_t VS1053::read_register ( uint8_t _reg ) const
{
  uint16_t result ;

  control_mode_on() ;
  SPI.write ( 3 ) ;                                // Read operation
  SPI.write ( _reg ) ;                             // Register to write (0..0xF)
  // Note: transfer16 does not seem to work
  result = ( SPI.transfer ( 0xFF ) << 8 ) |        // Read 16 bits data
           ( SPI.transfer ( 0xFF ) ) ;
  await_data_request() ;                           // Wait for DREQ to be HIGH again
  control_mode_off() ;
  return result ;
}

void VS1053::write_register ( uint8_t _reg, uint16_t _value ) const
{
  control_mode_on( );
  SPI.write ( 2 ) ;                                // Write operation
  SPI.write ( _reg ) ;                             // Register to write (0..0xF)
  SPI.write16 ( _value ) ;                         // Send 16 bits data
  await_data_request() ;
  control_mode_off() ;
}

bool VS1053::sdi_send_buffer ( uint8_t* data, size_t len )
{
  size_t chunk_length ;                            // Length of chunk 32 byte or shorter

  data_mode_on() ;
  while ( len )                                    // More to do?
  {
    chunk_length = len ;
    if ( len > vs1053_chunk_size )
    {
      chunk_length = vs1053_chunk_size ;
    }
    len -= chunk_length ;
    await_data_request() ;                         // Wait for space available
    SPI.writeBytes ( data, chunk_length ) ;
    data += chunk_length ;
  }
  data_mode_off() ;
  return data_request() ;                          // True if more data can de stored in fifo
}

void VS1053::sdi_send_fillers ( size_t len )
{
  size_t chunk_length ;                            // Length of chunk 32 byte or shorter

  data_mode_on() ;
  while ( len )                                    // More to do?
  {
    await_data_request() ;                         // Wait for space available
    chunk_length = len ;
    if ( len > vs1053_chunk_size )
    {
      chunk_length = vs1053_chunk_size ;
    }
    len -= chunk_length ;
    while ( chunk_length-- )
    {
      SPI.write ( endFillByte ) ;
    }
  }
  data_mode_off();
}

void VS1053::wram_write ( uint16_t address, uint16_t data )
{
  write_register ( SCI_WRAMADDR, address ) ;
  write_register ( SCI_WRAM, data ) ;
}

uint16_t VS1053::wram_read ( uint16_t address )
{
  write_register ( SCI_WRAMADDR, address ) ;            // Start reading from WRAM
  return read_register ( SCI_WRAM ) ;                   // Read back result
}

bool VS1053::testComm ( const char *header )
{
  // Test the communication with the VS1053 module.  The result wille be returned.
  // If DREQ is low, there is problably no VS1053 connected.  Pull the line HIGH
  // in order to prevent an endless loop waiting for this signal.  The rest of the
  // software will still work, but readbacks from VS1053 will fail.
  int            i ;                                    // Loop control
  uint16_t       r1, r2, cnt = 0 ;
  uint16_t       delta = 300 ;                          // 3 for fast SPI
  const uint16_t vstype[] = { 1001, 1011, 1002, 1003,   // Possible chip versions
                              1053, 1033, 0000, 1103 } ;
  
  dbgprint ( header ) ;                                 // Show a header
  if ( !digitalRead ( dreq_pin ) )
  {
    dbgprint ( "VS1053 not properly installed!" ) ;
    // Allow testing without the VS1053 module
    pinMode ( dreq_pin,  INPUT_PULLUP ) ;               // DREQ is now input with pull-up
    return false ;                                      // Return bad result
  }
  // Further TESTING.  Check if SCI bus can write and read without errors.
  // We will use the volume setting for this.
  // Will give warnings on serial output if DEBUG is active.
  // A maximum of 20 errors will be reported.
  if ( strstr ( header, "Fast" ) )
  {
    delta = 3 ;                                         // Fast SPI, more loops
  }
  for ( i = 0 ; ( i < 0xFFFF ) && ( cnt < 20 ) ; i += delta )
  {
    write_register ( SCI_VOL, i ) ;                     // Write data to SCI_VOL
    r1 = read_register ( SCI_VOL ) ;                    // Read back for the first time
    r2 = read_register ( SCI_VOL ) ;                    // Read back a second time
    if  ( r1 != r2 || i != r1 || i != r2 )              // Check for 2 equal reads
    {
      dbgprint ( "VS1053 SPI error. SB:%04X R1:%04X R2:%04X", i, r1, r2 ) ;
      cnt++ ;
      delay ( 10 ) ;
    }
  }
  okay = ( cnt == 0 ) ;                                 // True if working correctly
  // Further testing: is it the right chip?
  r1 = ( read_register ( SCI_STATUS ) >> 4 ) & 0x7 ;    // Read status to get the version
  if ( r1 !=  4 )                                       // Version 4 is a genuine VS1053
  {
    dbgprint ( "This is not a VS1053, "                 // Report the wrong chip
               "but a VS%d instead!",
               vstype[r1] ) ;
    okay = false ;
  }
  return ( okay ) ;                                     // Return the result
}

void VS1053::begin()
{
  pinMode      ( dreq_pin,  INPUT ) ;                   // DREQ is an input
  pinMode      ( cs_pin,    OUTPUT ) ;                  // The SCI and SDI signals
  pinMode      ( dcs_pin,   OUTPUT ) ;
  digitalWrite ( dcs_pin,   HIGH ) ;                    // Start HIGH for SCI en SDI
  digitalWrite ( cs_pin,    HIGH ) ;
  if ( shutdown_pin >= 0 )                              // Shutdown in use?
  {
    pinMode ( shutdown_pin,   OUTPUT ) ;
  }
  if ( shutdownx_pin >= 0 )                            // Shutdown (inversed logic) in use?
  {
    pinMode ( shutdownx_pin,   OUTPUT ) ;
  }
  output_enable ( false ) ;                            // Disable amplifier through shutdown pin(s)
  delay ( 100 ) ;
  // Init SPI in slow mode ( 0.2 MHz )
  VS1053_SPI = SPISettings ( 200000, MSBFIRST, SPI_MODE0 ) ;
  SPI.setDataMode ( SPI_MODE0 ) ;
  SPI.setBitOrder ( MSBFIRST ) ;
  //printDetails ( "Right after reset/startup" ) ;
  delay ( 20 ) ;
  //printDetails ( "20 msec after reset" ) ;
  if ( testComm ( "Slow SPI, Testing VS1053 read/write registers..." ) )
  {
    // Most VS1053 modules will start up in midi mode.  The result is that there is no audio
    // when playing MP3.  You can modify the board, but there is a more elegant way:
    wram_write ( 0xC017, 3 ) ;                            // GPIO DDR = 3
    wram_write ( 0xC019, 0 ) ;                            // GPIO ODATA = 0
    delay ( 100 ) ;
    //printDetails ( "After test loop" ) ;
    softReset() ;                                         // Do a soft reset
    // Switch on the analog parts
    write_register ( SCI_AUDATA, 44100 + 1 ) ;            // 44.1kHz + stereo
    // The next clocksetting allows SPI clocking at 5 MHz, 4 MHz is safe then.
    write_register ( SCI_CLOCKF, 6 << 12 ) ;              // Normal clock settings
    // multiplyer 3.0 = 12.2 MHz
    //SPI Clock to 4 MHz. Now you can set high speed SPI clock.
    VS1053_SPI = SPISettings ( 5000000, MSBFIRST, SPI_MODE0 ) ;
    write_register ( SCI_MODE, _BV ( SM_SDINEW ) | _BV ( SM_LINE1 ) ) ;
    testComm ( "Fast SPI, Testing VS1053 read/write registers again..." ) ;
    delay ( 10 ) ;
    await_data_request() ;
    endFillByte = wram_read ( 0x1E06 ) & 0xFF ;
    dbgprint ( "endFillByte is %X", endFillByte ) ;
    //printDetails ( "After last clocksetting" ) ;
    delay ( 100 ) ;
  }
}

void VS1053::setVolume ( uint8_t vol )
{
  // Set volume.  Both left and right.
  // Input value is 0..100.  100 is the loudest.
  // Clicking reduced by using 0xf8 to 0x00 as limits.
  uint16_t value ;                                      // Value to send to SCI_VOL

  if ( vol != curvol )
  {
    curvol = vol ;                                      // Save for later use
    value = map ( vol, 0, 100, 0xF8, 0x00 ) ;           // 0..100% to one channel
    value = ( value << 8 ) | value ;
    write_register ( SCI_VOL, value ) ;                 // Volume left and right
    if ( vol == 0 )                                     // Completely silence?
    {
      output_enable ( false ) ;                         // Yes, mute amplifier
    }
    else
    {
      if ( datamode != STOPPED )
      {
      output_enable ( true ) ;                          // Enable amplifier if not stopped
      }
    }
    output_enable ( vol != 0 ) ;                        // Enable/disable amplifier through shutdown pin(s)
  }
}

void VS1053::setTone ( uint8_t *rtone )                 // Set bass/treble (4 nibbles)
{
  // Set tone characteristics.  See documentation for the 4 nibbles.
  uint16_t value = 0 ;                                  // Value to send to SCI_BASS
  int      i ;                                          // Loop control

  for ( i = 0 ; i < 4 ; i++ )
  {
    value = ( value << 4 ) | rtone[i] ;                 // Shift next nibble in
  }
  write_register ( SCI_BASS, value ) ;                  // Volume left and right
}

void VS1053::startSong()
{
  sdi_send_fillers ( 10 ) ;
  output_enable ( true ) ;                              // Enable amplifier through shutdown pin(s)
}

bool VS1053::playChunk ( uint8_t* data, size_t len )
{
  return okay && sdi_send_buffer ( data, len ) ;        // True if more data can be added to fifo
}

void VS1053::stopSong()
{
  uint16_t modereg ;                                    // Read from mode register
  int      i ;                                          // Loop control

  sdi_send_fillers ( 2052 ) ;
  output_enable ( false ) ;                             // Disable amplifier through shutdown pin(s)
  delay ( 10 ) ;
  write_register ( SCI_MODE, _BV ( SM_SDINEW ) | _BV ( SM_CANCEL ) ) ;
  for ( i = 0 ; i < 20 ; i++ )
  {
    sdi_send_fillers ( 32 ) ;
    modereg = read_register ( SCI_MODE ) ;              // Read mode status
    if ( ( modereg & _BV ( SM_CANCEL ) ) == 0 )         // SM_CANCEL will be cleared when finished
    {
      sdi_send_fillers ( 2052 ) ;
      dbgprint ( "Song stopped correctly after %d msec", i * 10 ) ;
      return ;
    }
    delay ( 10 ) ;
  }
  printDetails ( "Song stopped incorrectly!" ) ;
}

void VS1053::softReset()
{
  write_register ( SCI_MODE, _BV ( SM_SDINEW ) | _BV ( SM_RESET ) ) ;
  delay ( 10 ) ;
  await_data_request() ;
}

void VS1053::printDetails ( const char *header )
{
  uint16_t     regbuf[16] ;
  uint8_t      i ;

  dbgprint ( header ) ;
  dbgprint ( "REG   Contents" ) ;
  dbgprint ( "---   -----" ) ;
  for ( i = 0 ; i <= SCI_num_registers ; i++ )
  {
    regbuf[i] = read_register ( i ) ;
  }
  for ( i = 0 ; i <= SCI_num_registers ; i++ )
  {
    delay ( 5 ) ;
    dbgprint ( "%3X - %5X", i, regbuf[i] ) ;
  }
}

void  VS1053::output_enable ( bool ena )               // Enable amplifier through shutdown pin(s)
{
  if ( shutdown_pin >= 0 )                             // Shutdown in use?
  {
    digitalWrite ( shutdown_pin, !ena ) ;              // Shut down or enable audio output
  }
  if ( shutdownx_pin >= 0 )                            // Shutdown (inversed logic) in use?
  {
    digitalWrite ( shutdownx_pin, ena ) ;              // Shut down or enable audio output
  }
}


void VS1053::AdjustRate ( long ppm2 )                  // Fine tune the data rate 
{
  write_register ( SCI_WRAMADDR, 0x1e07 ) ;
  write_register ( SCI_WRAM,     ppm2 ) ;
  write_register ( SCI_WRAM,     ppm2 >> 16 ) ;
  // oldClock4KHz = 0 forces  adjustment calculation when rate checked.
  write_register ( SCI_WRAMADDR, 0x5b1c ) ;
  write_register ( SCI_WRAM,     0 ) ;
  // Write to AUDATA or CLOCKF checks rate and recalculates adjustment.
  write_register ( SCI_AUDATA,   read_register ( SCI_AUDATA ) ) ;
}


// The object for the MP3 player
VS1053* vs1053player ;

//**************************************************************************************************
// End VS1053 stuff.                                                                               *
//**************************************************************************************************

// Include software for the right display
#ifdef BLUETFT
#include "bluetft.h"                                     // For ILI9163C or ST7735S 128x160 display
#endif
#ifdef ILI9341
#include "ILI9341.h"                                     // For ILI9341 320x240 display
#endif
#ifdef OLED
#include "SSD1306.h"                                     // For OLED I2C SD1306 64x128 display
#endif
#ifdef LCD1602I2C
#include "LCD1602.h"                                     // For LCD 1602 display (I2C)
#endif
#ifdef LCD2004I2C
#include "LCD2004.h"                                     // For LCD 2004 display (I2C)
#endif
#ifdef DUMMYTFT
#include "Dummytft.h"                                    // For Dummy display
#endif
#ifdef NEXTION
#include "NEXTION.h"                                     // For NEXTION display
#endif
//
// Include software for CH376
#include "CH376.h"                                       // For CH376 USB interface
// Include software for SD card
#include "SDcard.h"                                      // For SD caard interface

//**************************************************************************************************
//                                           B L S E T                                             *
//**************************************************************************************************
// Enable or disable the TFT backlight if configured.                                              *
// May be called from interrupt level.                                                             *
//**************************************************************************************************
void IRAM_ATTR blset ( bool enable )
{
  if ( ini_block.tft_bl_pin >= 0 )                       // Backlight for TFT control?
  {
    digitalWrite ( ini_block.tft_bl_pin, enable ) ;      // Enable/disable backlight
  }
  if ( ini_block.tft_blx_pin >= 0 )                      // Backlight for TFT (inversed logic) control?
  {
    digitalWrite ( ini_block.tft_blx_pin, !enable ) ;    // Enable/disable backlight
  }
  if ( enable )
  {
    bltimer = 0 ;                                        // Reset counter backlight time-out
  }
}


//**************************************************************************************************
//                                      N V S O P E N                                              *
//**************************************************************************************************
// Open Preferences with my-app namespace. Each application module, library, etc.                  *
// has to use namespace name to prevent key name collisions. We will open storage in               *
// RW-mode (second parameter has to be false).                                                     *
//**************************************************************************************************
void nvsopen()
{
  if ( ! nvshandle )                                         // Opened already?
  {
    nvserr = nvs_open ( NAME, NVS_READWRITE, &nvshandle ) ;  // No, open nvs
    if ( nvserr )
    {
      dbgprint ( "nvs_open failed!" ) ;
    }
  }
}


//**************************************************************************************************
//                                      N V S C L E A R                                            *
//**************************************************************************************************
// Clear all preferences.                                                                          *
//**************************************************************************************************
esp_err_t nvsclear()
{
  nvsopen() ;                                         // Be sure to open nvs
  return nvs_erase_all ( nvshandle ) ;                // Clear all keys
}


//**************************************************************************************************
//                                      N V S G E T S T R                                          *
//**************************************************************************************************
// Read a string from nvs.                                                                         *
//**************************************************************************************************
String nvsgetstr ( const char* key )
{
  static char   nvs_buf[NVSBUFSIZE] ;       // Buffer for contents
  size_t        len = NVSBUFSIZE ;          // Max length of the string, later real length

  nvsopen() ;                               // Be sure to open nvs
  nvs_buf[0] = '\0' ;                       // Return empty string on error
  nvserr = nvs_get_str ( nvshandle, key, nvs_buf, &len ) ;
  if ( nvserr )
  {
    dbgprint ( "nvs_get_str failed %X for key %s, keylen is %d, len is %d!",
               nvserr, key, strlen ( key), len ) ;
    dbgprint ( "Contents: %s", nvs_buf ) ;
  }
  return String ( nvs_buf ) ;
}


//**************************************************************************************************
//                                      N V S S E T S T R                                          *
//**************************************************************************************************
// Put a key/value pair in nvs.  Length is limited to allow easy read-back.                        *
// No writing if no change.                                                                        *
//**************************************************************************************************
esp_err_t nvssetstr ( const char* key, String val )
{
  String curcont ;                                         // Current contents
  bool   wflag = true  ;                                   // Assume update or new key

  dbgprint ( "Setstring for %s: %s", key, val.c_str() ) ;
  if ( val.length() >= NVSBUFSIZE )                        // Limit length of string to store
  {
    dbgprint ( "nvssetstr length failed!" ) ;
    return ESP_ERR_NVS_NOT_ENOUGH_SPACE ;
  }
  if ( nvssearch ( key ) )                                 // Already in nvs?
  {
    curcont = nvsgetstr ( key ) ;                          // Read current value
    wflag = ( curcont != val ) ;                           // Value change?
  }
  if ( wflag )                                             // Update or new?
  {
    dbgprint ( "nvssetstr update value" ) ;
    nvserr = nvs_set_str ( nvshandle, key, val.c_str() ) ; // Store key and value
    if ( nvserr )                                          // Check error
    {
      dbgprint ( "nvssetstr failed!" ) ;
    }
  }
  return nvserr ;
}


//**************************************************************************************************
//                                      N V S C H K E Y                                            *
//**************************************************************************************************
// Change a keyname in in nvs.                                                                     *
//**************************************************************************************************
void nvschkey ( const char* oldk, const char* newk )
{
  String curcont ;                                         // Current contents

  if ( nvssearch ( oldk ) )                                // Old key in nvs?
  {
    curcont = nvsgetstr ( oldk ) ;                         // Read current value
    nvs_erase_key ( nvshandle, oldk ) ;                    // Remove key
    nvssetstr ( newk, curcont ) ;                          // Insert new
  }
}


//**************************************************************************************************
//                                      C L A I M S P I                                            *
//**************************************************************************************************
// Claim the SPI bus.  Uses FreeRTOS semaphores.                                                   *
// If the semaphore cannot be claimed within the time-out period, the function continues without   *
// claiming the semaphore.  This is incorrect but allows debugging.                                *
//**************************************************************************************************
void claimSPI ( const char* p )
{
  const              TickType_t ctry = 10 ;                 // Time to wait for semaphore
  uint32_t           count = 0 ;                            // Wait time in ticks
  static const char* old_id = "none" ;                      // ID that holds the bus

  while ( xSemaphoreTake ( SPIsem, ctry ) != pdTRUE  )      // Claim SPI bus
  {
    if ( count++ > 25 )
    {
      dbgprint ( "SPI semaphore not taken within %d ticks by CPU %d, id %s",
                 count * ctry,
                 xPortGetCoreID(),
                 p ) ;
      dbgprint ( "Semaphore is claimed by %s", old_id ) ;
    }
    if ( count >= 100 )
    {
      return ;                                               // Continue without semaphore
    }
  }
  old_id = p ;                                               // Remember ID holding the semaphore
}


//**************************************************************************************************
//                                   R E L E A S E S P I                                           *
//**************************************************************************************************
// Free the the SPI bus.  Uses FreeRTOS semaphores.                                                *
//**************************************************************************************************
void releaseSPI()
{
  xSemaphoreGive ( SPIsem ) ;                            // Release SPI bus
}


//**************************************************************************************************
//                                      Q U E U E F U N C                                          *
//**************************************************************************************************
// Queue a special function for the play task.                                                     *
//**************************************************************************************************
void queuefunc ( int func )
{
  qdata_struct     specchunk ;                          // Special function to queue

  specchunk.datatyp = func ;                            // Put function in datatyp
  xQueueSendToFront ( dataqueue, &specchunk, 200 ) ;    // Send to queue (First Out)
}


//**************************************************************************************************
//                                      N V S S E A R C H                                          *
//**************************************************************************************************
// Check if key exists in nvs.                                                                     *
//**************************************************************************************************
bool nvssearch ( const char* key )
{
  size_t        len = NVSBUFSIZE ;                      // Length of the string

  nvsopen() ;                                           // Be sure to open nvs
  nvserr = nvs_get_str ( nvshandle, key, NULL, &len ) ; // Get length of contents
  return ( nvserr == ESP_OK ) ;                         // Return true if found
}


//**************************************************************************************************
//                                      T F T S E T                                                *
//**************************************************************************************************
// Request to display a segment on TFT.  Version for char* and String parameter.                   *
//**************************************************************************************************
void tftset ( uint16_t inx, const char *str )
{
  if ( inx < TFTSECS )                                  // Segment available on display
  {
    if ( str )                                          // String specified?
    {
      tftdata[inx].str = String ( str ) ;               // Yes, set string
    }
    tftdata[inx].update_req = true ;                    // and request flag
  }
}

void tftset ( uint16_t inx, String& str )
{
  if ( inx < TFTSECS )                                  // Segment available on display
  {
    tftdata[inx].str = str ;                            // Set string
    tftdata[inx].update_req = true ;                    // and request flag
  }
}


//**************************************************************************************************
//                                      U T F 8 A S C I I                                          *
//**************************************************************************************************
// UTF8-Decoder: convert UTF8-string to extended ASCII.                                            *
// Convert a single Character from UTF8 to Extended ASCII.                                         *
// Return "0" if a byte has to be ignored.                                                         *
//**************************************************************************************************
char utf8ascii ( char ascii )
{
  static const char lut_C3[] = { "AAAAAAACEEEEIIIIDNOOOOO#0UUUU###"
                                 "aaaaaaaceeeeiiiidnooooo##uuuuyyy" } ;
  static char       c1 ;              // Last character buffer
  char              res = '\0' ;      // Result, default 0

  if ( ascii <= 0x7F )                // Standard ASCII-set 0..0x7F handling
  {
    c1 = 0 ;
    res = ascii ;                     // Return unmodified
  }
  else
  {
    switch ( c1 )                     // Conversion depending on first UTF8-character
    {
      case 0xC2: res = '~' ;
        break ;
      case 0xC3: res = lut_C3[ascii - 128] ;
        break ;
      case 0x82: if ( ascii == 0xAC )
        {
          res = 'E' ;                 // Special case Euro-symbol
        }
    }
    c1 = ascii ;                      // Remember actual character
  }
  return res ;                        // Otherwise: return zero, if character has to be ignored
}


//**************************************************************************************************
//                                U T F 8 A S C I I _ I P                                          *
//**************************************************************************************************
// In Place conversion UTF8-string to Extended ASCII (ASCII is shorter!).                          *
//**************************************************************************************************
void utf8ascii_ip ( char* s )
{
  int  i, k = 0 ;                     // Indexes for in en out string
  char c ;

  for ( i = 0 ; s[i] ; i++ )          // For every input character
  {
    c = utf8ascii ( s[i] ) ;          // Translate if necessary
    if ( c )                          // Good translation?
    {
      s[k++] = c ;                    // Yes, put in output string
    }
  }
  s[k] = 0 ;                          // Take care of delimeter
}


//**************************************************************************************************
//                                      U T F 8 A S C I I                                          *
//**************************************************************************************************
// Conversion UTF8-String to Extended ASCII String.                                                *
//**************************************************************************************************
String utf8ascii ( const char* s )
{
  int  i ;                            // Index for input string
  char c ;
  String res = "" ;                   // Result string

  for ( i = 0 ; s[i] ; i++ )          // For every input character
  {
    c = utf8ascii ( s[i] ) ;          // Translate if necessary
    if ( c )                          // Good translation?
    {
      res += String ( c ) ;           // Yes, put in output string
    }
  }
  return res ;
}


//**************************************************************************************************
//                                          D B G P R I N T                                        *
//**************************************************************************************************
// Send a line of info to serial output.  Works like vsprintf(), but checks the DEBUG flag.        *
// Print only if DEBUG flag is true.  Always returns the formatted string.                         *
//**************************************************************************************************
char* dbgprint ( const char* format, ... )
{
  static char sbuf[DEBUG_BUFFER_SIZE] ;                // For debug lines
  va_list varArgs ;                                    // For variable number of params

  va_start ( varArgs, format ) ;                       // Prepare parameters
  vsnprintf ( sbuf, sizeof(sbuf), format, varArgs ) ;  // Format the message
  va_end ( varArgs ) ;                                 // End of using parameters
  if ( DEBUG )                                         // DEBUG on?
  {
    Serial.print ( "D: " ) ;                           // Yes, print prefix
      Serial.println ( sbuf ) ;                        //   yes, print info to Serial
      Serial.flush ( ) ;
  }
  return sbuf ;                                        // Return stored string
}


//**************************************************************************************************
//                                     G E T E N C R Y P T I O N T Y P E                           *
//**************************************************************************************************
// Read the encryption type of the network and return as a 4 byte name                             *
//**************************************************************************************************
const char* getEncryptionType ( wifi_auth_mode_t thisType )
{
  switch ( thisType )
  {
    case WIFI_AUTH_OPEN:
      return "OPEN" ;
    case WIFI_AUTH_WEP:
      return "WEP" ;
    case WIFI_AUTH_WPA_PSK:
      return "WPA_PSK" ;
    case WIFI_AUTH_WPA2_PSK:
      return "WPA2_PSK" ;
    case WIFI_AUTH_WPA_WPA2_PSK:
      return "WPA_WPA2_PSK" ;
    case WIFI_AUTH_MAX:
      return "MAX" ;
    default:
      break ;
  }
  return "????" ;
}


//**************************************************************************************************
//                                        L I S T N E T W O R K S                                  *
//**************************************************************************************************
// List the available networks.                                                                    *
// Acceptable networks are those who have an entry in the preferences.                             *
// SSIDs of available networks will be saved for use in webinterface.                              *
//**************************************************************************************************
void listNetworks()
{
  WifiInfo_t       winfo ;            // Entry from wifilist
  wifi_auth_mode_t encryption ;       // TKIP(WPA), WEP, etc.
  const char*      acceptable ;       // Netwerk is acceptable for connection
  int              i, j ;             // Loop control

  dbgprint ( "Scan Networks" ) ;                         // Scan for nearby networks
  numSsid = WiFi.scanNetworks() ;
  dbgprint ( "Scan completed" ) ;
  if ( numSsid <= 0 )
  {
    dbgprint ( "Couldn't get a wifi connection" ) ;
    return ;
  }
  // print the list of networks seen:
  dbgprint ( "Number of available networks: %d",
             numSsid ) ;
  // Print the network number and name for each network found and
  for ( i = 0 ; i < numSsid ; i++ )
  {
    acceptable = "" ;                                    // Assume not acceptable
    for ( j = 0 ; j < wifilist.size() ; j++ )            // Search in wifilist
    {
      winfo = wifilist[j] ;                              // Get one entry
      if ( WiFi.SSID(i).indexOf ( winfo.ssid ) == 0 )    // Is this SSID acceptable?
      {
        acceptable = "Acceptable" ;
        break ;
      }
    }
    encryption = WiFi.encryptionType ( i ) ;
    dbgprint ( "%2d - %-25s Signal: %3d dBm, Encryption %4s, %s",
               i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i),
               getEncryptionType ( encryption ),
               acceptable ) ;
    // Remember this network for later use
    networks += WiFi.SSID(i) + String ( "|" ) ;
  }
  dbgprint ( "End of list" ) ;
}


//**************************************************************************************************
//                                          T I M E R 1 0 S E C                                    *
//**************************************************************************************************
// Extra watchdog.  Called every 10 seconds.                                                       *
// If totalcount has not been changed, there is a problem and playing will stop.                   *
// Note that calling timely procedures within this routine or in called functions will             *
// cause a crash!                                                                                  *
//**************************************************************************************************
void IRAM_ATTR timer10sec()
{
  static uint32_t oldtotalcount = 7321 ;          // Needed for change detection
  static uint8_t  morethanonce = 0 ;              // Counter for succesive fails
  uint32_t        bytesplayed ;                   // Bytes send to MP3 converter

  if ( datamode & ( INIT | HEADER | DATA |        // Test op playing
                    METADATA | PLAYLISTINIT |
                    PLAYLISTHEADER |
                    PLAYLISTDATA ) )
  {
    bytesplayed = totalcount - oldtotalcount ;    // Nunber of bytes played in the 10 seconds
    oldtotalcount = totalcount ;                  // Save for comparison in next cycle
    if ( bytesplayed == 0 )                       // Still playing?
    {
      if ( morethanonce > 10 )                    // No! Happened too many times?
      {
        ESP.restart() ;                           // Reset the CPU, probably no return
      }
      if ( datamode & ( PLAYLISTDATA |            // In playlist mode?
                        PLAYLISTINIT |
                        PLAYLISTHEADER ) )
      {
        playlist_num = 0 ;                        // Yes, end of playlist
      }
      if ( ( morethanonce > 0 ) ||                // Happened more than once?
           ( playlist_num > 0 ) )                 // Or playlist active?
      {
        datamode = STOPREQD ;                     // Stop player
        ini_block.newpreset++ ;                   // Yes, try next channel
      }
      morethanonce++ ;                            // Count the fails
    }
    else
    {
      //                                          // Data has been send to MP3 decoder
      // Bitrate in kbits/s is bytesplayed / 10 / 1000 * 8
      mbitrate = ( bytesplayed + 625 ) / 1250 ;   // Measured bitrate
      morethanonce = 0 ;                          // Data seen, reset failcounter
    }
  }
}


//**************************************************************************************************
//                                          T I M E R 1 0 0                                        *
//**************************************************************************************************
// Called every 100 msec on interrupt level, so must be in IRAM and no lengthy operations          *
// allowed.                                                                                        *
//**************************************************************************************************
void IRAM_ATTR timer100()
{
  sv int16_t   count10sec = 0 ;                   // Counter for activatie 10 seconds process
  sv int16_t   eqcount = 0 ;                      // Counter for equal number of clicks
  sv int16_t   oldclickcount = 0 ;                // To detect difference

  if ( ++count10sec == 100  )                     // 10 seconds passed?
  {
    timer10sec() ;                                // Yes, do 10 second procedure
    count10sec = 0 ;                              // Reset count
  }
  if ( ( count10sec % 10 ) == 0 )                 // One second over?
  {
    scaniocount = scanios ;                       // TEST*TEST*TEST
    scanios = 0 ;
    if ( ++timeinfo.tm_sec >= 60 )                // Yes, update number of seconds
    {
      timeinfo.tm_sec = 0 ;                       // Wrap after 60 seconds
      if ( ++timeinfo.tm_min >= 60 )
      {
        timeinfo.tm_min = 0 ;                     // Wrap after 60 minutes
        if ( ++timeinfo.tm_hour >= 24 )
        {
          timeinfo.tm_hour = 0 ;                  // Wrap after 24 hours
        }
      }
    }
    time_req = true ;                             // Yes, show current time request
    if ( ++bltimer == BL_TIME )                   // Time to blank the TFT screen?
    {
      bltimer = 0 ;                               // Yes, reset counter
      blset ( false ) ;                           // Disable TFT (backlight)
    }
  }
  // Handle rotary encoder. Inactivity counter will be reset by encoder interrupt
  if ( ++enc_inactivity == 36000 )                // Count inactivity time
  {
    enc_inactivity = 1000 ;                       // Prevent wrap
  }
  // Now detection of single/double click of rotary encoder switch
  if ( clickcount )                               // Any click?
  {
    if ( oldclickcount == clickcount )            // Yes, stable situation?
    {
      if ( ++eqcount == 4 )                       // Long time stable?
      {
        eqcount = 0 ;
        if ( clickcount > 2 )                     // Triple click?
        {
          tripleclick = true ;                    // Yes, set result
        }
        else if ( clickcount == 2 )               // Double click?
        {
          doubleclick = true ;                    // Yes, set result
        }
        else
        {
          singleclick = true ;                    // Just one click seen
        }
        clickcount = 0 ;                          // Reset number of clicks
      }
    }
    else
    {
      oldclickcount = clickcount ;                // To detect change
      eqcount = 0 ;                               // Not stable, reset count
    }
  }
}


//**************************************************************************************************
//                             D E C O D E R _I R R C 5                                            *
//**************************************************************************************************
// Routine for decoding Philips IR Protocol RC5, called from ISR.                                  *
// Parameter intval holds the length of the last pulse (in microseconds), t0 the current time (in  *
// microseconds and pin_state the current state of the IR input pin.                               *
// Algorithm adopted from https://github.com/pinkeen/avr-rc5/blob/master/rc5.c                     *
// Sets the IR-data for later processing in scanIR()                                               *
// (Toggle bit in the protocol frame is cleared before reporting result to application             *
//**************************************************************************************************
void IRAM_ATTR decoder_IRRC5(uint32_t intval, uint32_t t0, int pin_state)
{
//#define RC5SHORT_MIN 444   /* 444 microseconds */
//#define RC5SHORT_MAX 1333  /* 1333 microseconds */
//#define RC5LONG_MIN 1334   /* 1334 microseconds */
//#define RC5LONG_MAX 2222   /* 2222 microseconds */
    const uint8_t trans[4] = {0x01, 0x91, 0x9b, 0xfb} ; // state transition table
    sv uint8_t rc5_counter = 14 ;                       // Bit counter (counting downwards)
    sv uint16_t rc5_command = 0 ;                       // The command received 
    sv RC5State rc5_state = RC5STATE_BEGIN ;            // State of the RC5 decoder statemachine
    sv uint32_t rc5_lasttime = 0 ;                      // last time a valid command has been received 
    /* VS1838B pulls the data line up, giving active low,
     * so the output is inverted. If data pin is high then the edge
     * was falling and vice versa.
     * 
     *  Event numbers:
     *  0 - short space
     *  2 - short pulse
     *  4 - long space
     *  6 - long pulse
     */

    uint8_t event = pin_state? 2 : 0 ; 
    if(intval > 1333 && intval <= 2222)  // "Long" event detected?
    {
        event += 4 ;
    } 
    else if(intval < 444 || intval > 2222)
    {
        /* If delay wasn't long and isn't short then
         * it is erroneous so we need to reset but
         * we don't return from interrupt so we don't
         * loose the edge currently detected. */
      rc5_counter = 14 ;                   // Bit counter (counting downwards)
      rc5_command = 0 ;                    // The command received 
      rc5_state = RC5STATE_BEGIN ;         // State of the RC5 decoder statemachine
    }
    if(rc5_state == RC5STATE_BEGIN)         // First bit?
    {
        rc5_counter-- ;          
        rc5_command = 1 ;                   // Is always 1
        rc5_state = RC5STATE_MID1 ;         // State: Middle of HIGH bit detected
    } 
    else 
    {
      RC5State rc5_newstate = (RC5State)((trans[rc5_state] >> event) & 0x03);
      
      if (rc5_newstate == rc5_state || rc5_state > RC5STATE_START0)
      {
        /* No state change or wrong state means
         * error so reset. */
        rc5_counter = 14 ;                   // Bit counter (counting downwards)
        rc5_command = 0 ;                    // The command received 
        rc5_state = RC5STATE_BEGIN ;         // State of the RC5 decoder statemachine
      }
      else
      {    
        rc5_state = rc5_newstate ;
    
       /* Emit 0 or 1 to rc5_command */
        if ( (rc5_state == RC5STATE_MID0) || (rc5_state == RC5STATE_MID1))
        {
          rc5_counter-- ;
          rc5_command = rc5_command << 1 ;
          if (rc5_state == RC5STATE_MID1)
            rc5_command++ ;
        }    
        /* The only valid end states are MID0 and START1.
        * Mid0 is ok, but if we finish in MID1 we need to wait
        * for START1 so the last edge is consumed. */
        if(rc5_counter == 0 && (rc5_state == RC5STATE_START1 || rc5_state == RC5STATE_MID0))
        {
          rc5_state = RC5STATE_END ;
          rc5_command = (rc5_command & 0x37ff) ; // + ((rc5_command & 0x1000) >> 1)+ 0xf000;
          if (ir.protocol == IR_NONE) 
          {
            if ((rc5_command == ir.command) && (t0 - rc5_lasttime < 220000))
              ir.repeat_counter++ ;
            else 
            {
              ir.exitcommand = ir.command ;
              ir.command = rc5_command ;
              ir.repeat_counter = 0 ;
            }
            ir.protocol = IR_RC5 ; 
          }
          rc5_lasttime = t0;
        }
      }
    }          
}


//**************************************************************************************************
//                             D E C O D E R _I R N E C                                            *
//**************************************************************************************************
// Routine for decoding NEC IR Protocol, called from ISR.                                          *
// Parameter intval holds the length of the last pulse (in microseconds), t0 the current time (in  *
// microseconds).                                                                                  *
// Intervals are 640 or 1640 microseconds for data.  syncpulses are 3400 micros or longer.         *
// Input is complete after 65 level changes.                                                       *
// Only the last 32 level changes are significant and will be handed over to common data.          *
//**************************************************************************************************
void decoder_IRNEC ( uint32_t intval, uint32_t t0 ) 
{

  sv uint32_t      ir_locvalue = 0 ;                 // IR code
  sv int           ir_loccount = 0 ;                 // Length of code
  sv uint32_t      ir_lasttime = 0;                  // Last time of valid code (either first shot or repeat)
  sv uint8_t       repeat_step = 0;                  // Current Status of repeat code received
  uint32_t         mask_in = 2 ;                     // Mask input for conversion
  uint16_t         mask_out = 1 ;                    // Mask output for conversion
  if ( ( intval > 300 ) && ( intval < 800 ) )        // Short pulse?
  {
    if ( repeat_step == 2 )                          // Full preamble for repeat seen?
    {  
      if (t0 - ir_lasttime < 120000)                 // Repeat shot is expected 108 ms either after last
      {                                              // initial key or previous repeat  
        if (ir.protocol == IR_NONE)                  // Last IR value has been consumed? repeatcode is valid?
        {
          ir.repeat_counter++ ;                      // Increment repeat counter (ir.command should still be valid)
          ir.protocol = IR_NEC;                      // Set protocol info to indicate valid data has been received
        }                                            // (repeat counter is in upper word of ir_value) 
        ir_lasttime = t0 ;                           // Reset timer for next repeat shot.
      }
      repeat_step = 0 ;                              // Reset repeat search state  
    } 
    else 
    {
      ir_locvalue = ir_locvalue << 1 ;               // Shift in a "zero" bit
      ir_loccount++ ;                                // Count number of received bits
      ir_0 = ( ir_0 * 3 + intval ) / 4 ;             // Compute average durartion of a short pulse
  }
  }
  else if ( ( intval > 1400 ) && ( intval < 1900 ) ) // Long pulse?
  {
    ir_locvalue = ( ir_locvalue << 1 ) + 1 ;         // Shift in a "one" bit
    ir_loccount++ ;                                  // Count number of received bits
    ir_1 = ( ir_1 * 3 + intval ) / 4 ;               // Compute average durartion of a short pulse
  }
  else if ( ir_loccount == 65 )                      // Value is correct after 65 level changes
  {
    ir.exitcommand = ir.command ;
    ir.command = 0 ;
    while ( mask_in )                                // Convert 32 bits to 16 bits
    {
      if ( ir_locvalue & mask_in )                   // Bit set in pattern?
      {
        ir.command |= mask_out ;                     // Set set bit in result
      }
      mask_in <<= 2 ;                                // Shift input mask 2 positions
      mask_out <<= 1 ;                               // Shift output mask 1 position
    }
    ir_loccount = 0 ;                                // Ready for next input
    ir.repeat_counter = 0 ;                          // Report as first press
    ir.protocol = IR_NEC;                            // Set protocol info to indicate valid data has been received
    ir_lasttime = t0 ;                               // Start timer for detecting valid repeat shots
  }
  else
  {
    if ((intval > 8500) && (intval < 9500) && (repeat_step == 0))         // Check for repeat: a repeat shot starts 
      repeat_step++;                                                      // with a 9ms pulse
    else if ((intval > 2000) && (intval < 2500) && (repeat_step == 1))    // followed by a 2.25ms pulse
      repeat_step++;
    else
      repeat_step = 0;                                                    // Reset repeat shot decoding
    ir_locvalue = 0 ;                                // Reset decoding
    ir_loccount = 0 ;
  }
  
}

//**************************************************************************************************
//                                          I S R _ I R                                            *
//**************************************************************************************************
// Interrupts received from VS1838B on every change of the signal.                                 *
// This routine will call both decoders for NEC and RC5 protocols.                                 *
//**************************************************************************************************
void IRAM_ATTR isr_IR()
{
  sv uint32_t      t0 = 0 ;                          // To get the interval
  uint32_t         t1, intval ;                      // Current time and interval since last change
  int pin_state = digitalRead(ini_block.ir_pin);     // Get current state of InputPin

  t1 = micros() ;                                    // Get current time
  intval = t1 - t0 ;                                 // Compute interval
  t0 = t1 ;                                          // Save for next compare

  decoder_IRRC5 ( intval, t0, pin_state ) ;          // Run decoder for Philips RC5 protocol 

  decoder_IRNEC ( intval, t0 ) ;                     // Run decoder for NEC protocol
}

//**************************************************************************************************
//                                          I S R _ I R N E C                                      *
//**************************************************************************************************
// Interrupts received from VS1838B on every change of the signal.                                 *
// This routine will call the decoder for NEC protocol only.                                       *
//**************************************************************************************************
void IRAM_ATTR isr_IRNEC()
{
  sv uint32_t      t0 = 0 ;                          // To get the interval
  uint32_t         t1, intval ;                      // Current time and interval since last change
  int pin_state = digitalRead(ini_block.ir_pin);     // Get current state of InputPin

  t1 = micros() ;                                    // Get current time
  intval = t1 - t0 ;                                 // Compute interval
  t0 = t1 ;                                          // Save for next compare


  decoder_IRNEC ( intval, t0 ) ;                     // Run decoder for NEC protocol
}


//**************************************************************************************************
//                                          I S R _ I R R C 5                                      *
//**************************************************************************************************
// Interrupts received from VS1838B on every change of the signal.                                 *
// This routine will call the decoder for RC5 protocol only.                                       *
//**************************************************************************************************
void IRAM_ATTR isr_IRRC5()
{
  sv uint32_t      t0 = 0 ;                          // To get the interval
  uint32_t         t1, intval ;                      // Current time and interval since last change
  int pin_state = digitalRead(ini_block.ir_pin);     // Get current state of InputPin

  t1 = micros() ;                                    // Get current time
  intval = t1 - t0 ;                                 // Compute interval
  t0 = t1 ;                                          // Save for next compare

  decoder_IRRC5 ( intval, t0, pin_state ) ;          // Run decoder for Philips RC5 protocol 

}


/*
//**************************************************************************************************
//                                          I S R _ I R                                            *
//**************************************************************************************************
// Interrupts received from VS1838B on every change of the signal.                                 *
// This routine will call both decoders for NEC and RC5 protocols.                                 *
// Intervals are 640 or 1640 microseconds for data.  syncpulses are 3400 micros or longer.         *
// Input is complete after 65 level changes.                                                       *
// Only the last 32 level changes are significant and will be handed over to common data.          *
//**************************************************************************************************
void IRAM_ATTR isr_IR()
{
  sv uint32_t      t0 = 0 ;                          // To get the interval
  sv uint32_t      ir_locvalue = 0 ;                 // IR code
  sv int           ir_loccount = 0 ;                 // Length of code
  sv uint16_t      ir_repeatcode = 0;                // Code to report, if repeat shot has been detected
  sv uint32_t      ir_lasttime = 0;                  // Last time of valid code (either first shot or repeat)
  sv uint8_t       repeat_step = 0;                  // Current Status of repeat code received
  uint32_t         t1, intval ;                      // Current time and interval since last change
  uint32_t         mask_in = 2 ;                     // Mask input for conversion
  uint16_t         mask_out = 1 ;                    // Mask output for conversion

  t1 = micros() ;                                    // Get current time
  intval = t1 - t0 ;                                 // Compute interval
  t0 = t1 ;                                          // Save for next compare
  if ( ( intval > 300 ) && ( intval < 800 ) )        // Short pulse?
  {
    if (repeatStep == 2)                             // Preamble for repeat seen?
    {  
      if (t1 - ir_lasttime < 120000)                 // Repaet shot is expected after 108 ms (some margin)
      {  
        if ((ir_value == 0) && (ir_repeatcode != 0)) // Last IR value has been consumed?
          ir_value = ir_repeatcode;                  // If so, report repeat 
        ir_lasttime = t1;                            // Reset timer for next repeat shot.
      }
      repeatStep = 0;                                // Reset repeat search state  
    } 
    else 
    {
      ir_locvalue = ir_locvalue << 1 ;                 // Shift in a "zero" bit
      ir_loccount++ ;                                  // Count number of received bits
      ir_0 = ( ir_0 * 3 + intval ) / 4 ;               // Compute average durartion of a short pulse
    }
  }
  else if ( ( intval > 1400 ) && ( intval < 1900 ) ) // Long pulse?
  {
    ir_locvalue = ( ir_locvalue << 1 ) + 1 ;         // Shift in a "one" bit
    ir_loccount++ ;                                  // Count number of received bits
    ir_1 = ( ir_1 * 3 + intval ) / 4 ;               // Compute average durartion of a short pulse
  }
  else if ( ir_loccount == 65 )                      // Value is correct after 65 level changes
  {
    while ( mask_in )                                // Convert 32 bits to 16 bits
    {
      if ( ir_locvalue & mask_in )                   // Bit set in pattern?
      {
        ir_value |= mask_out ;                       // Set set bit in result
      }
      mask_in <<= 2 ;                                // Shift input mask 2 positions
      mask_out <<= 1 ;                               // Shift output mask 1 position
    }
    ir_repeatcode = (ir_value & 0xff00) | 
                ((ir_value & 0xff00) >> 8);          // Remember for repeat press
    if (0xff != (ir_value ^ ir_repeatcode))          // Extended NEC protocol code?
      ir_repeatcode = 0;                             // Do not report repeats. 
    ir_lasttime = t1;                                // Start timer for detecting valid repeat shots
    ir_loccount = 0 ;                                // Ready for next input
  }
  else
  {
    if ((intval > 8000) && (intval < 10000) && (repeatStep == 0))         // Check for repeat: a repeat shot starts 
      repeatStep = 1;                                                     // with a 9ms pulse
    else if ((intval > 2000) && (intval < 2500) && (repeatStep == 1))     // followed by a 2.25ms pulse
      repeatStep = 2;
    else
      repeatStep = 0;                                                     // Reset repeat shot decoding
    ir_locvalue = 0 ;                                // Reset decoding
    ir_loccount = 0 ;
  }
}
*/

//**************************************************************************************************
//                                          I S R _ E N C _ S W I T C H                            *
//**************************************************************************************************
// Interrupts received from rotary encoder switch.                                                 *
//**************************************************************************************************
void IRAM_ATTR isr_enc_switch()
{
  sv uint32_t     oldtime = 0 ;                            // Time in millis previous interrupt
  sv bool         sw_state ;                               // True is pushed (LOW)
  bool            newstate ;                               // Current state of input signal
  uint32_t        newtime ;                                // Current timestamp

  // Read current state of SW pin
  newstate = ( digitalRead ( ini_block.enc_sw_pin ) == LOW ) ;
  newtime = millis() ;
  if ( newtime == oldtime )                                // Debounce
  {
    return ;
  }
  if ( newstate != sw_state )                              // State changed?
  {
    sw_state = newstate ;                                  // Yes, set current (new) state
    if ( !sw_state )                                       // SW released?
    {
      if ( ( newtime - oldtime ) > 1000 )                  // More than 1 second?
      {
        longclick = true ;                                 // Yes, register longclick
      }
      else
      {
        clickcount++ ;                                     // Yes, click detected
      }
      enc_inactivity = 0 ;                                 // Not inactive anymore
    }
  }
  oldtime = newtime ;                                      // For next compare
}


//**************************************************************************************************
//                                          I S R _ E N C _ T U R N                                *
//**************************************************************************************************
// Interrupts received from rotary encoder (clk signal) knob turn.                                 *
// The encoder is a Manchester coded device, the outcomes (-1,0,1) of all the previous state and   *
// actual state are stored in the enc_states[].                                                    *
// Full_status is a 4 bit variable, the upper 2 bits are the previous encoder values, the lower    *
// ones are the actual ones.                                                                       *
// 4 bits cover all the possible previous and actual states of the 2 PINs, so this variable is     *
// the index enc_states[].                                                                         *
// No debouncing is needed, because only the valid states produce values different from 0.         *
// Rotation is 4 if position is moved from one fixed position to the next, so it is devided by 4.  *
//**************************************************************************************************
void IRAM_ATTR isr_enc_turn()
{
  sv uint32_t     old_state = 0x0001 ;                          // Previous state
  sv int16_t      locrotcount = 0 ;                             // Local rotation count
  uint8_t         act_state = 0 ;                               // The current state of the 2 PINs
  uint8_t         inx ;                                         // Index in enc_state
  sv const int8_t enc_states [] =                               // Table must be in DRAM (iram safe)
  { 0,                    // 00 -> 00
    -1,                   // 00 -> 01                           // dt goes HIGH
    1,                    // 00 -> 10
    0,                    // 00 -> 11
    1,                    // 01 -> 00                           // dt goes LOW
    0,                    // 01 -> 01
    0,                    // 01 -> 10
    -1,                   // 01 -> 11                           // clk goes HIGH
    -1,                   // 10 -> 00                           // clk goes LOW
    0,                    // 10 -> 01
    0,                    // 10 -> 10
    1,                    // 10 -> 11                           // dt goes HIGH
    0,                    // 11 -> 00
    1,                    // 11 -> 01                           // clk goes LOW
    -1,                   // 11 -> 10                           // dt goes HIGH
    0                     // 11 -> 11
  } ;
  // Read current state of CLK, DT pin. Result is a 2 bit binary number: 00, 01, 10 or 11.
  act_state = ( digitalRead ( ini_block.enc_clk_pin ) << 1 ) +
              digitalRead ( ini_block.enc_dt_pin ) ;
  inx = ( old_state << 2 ) + act_state ;                        // Form index in enc_states
  locrotcount += enc_states[inx] ;                              // Get delta: 0, +1 or -1
  if ( locrotcount == 4 )
  {
    rotationcount++ ;                                           // Divide by 4
    locrotcount = 0 ;
  }
  else if ( locrotcount == -4 )
  {
    rotationcount-- ;                                           // Divide by 4
    locrotcount = 0 ;
  }
  old_state = act_state ;                                       // Remember current status
  enc_inactivity = 0 ;
}


//**************************************************************************************************
//                                S E L E C T N E X T F S N O D E                                  *
//**************************************************************************************************
// Select the next or previous mp3 file from USB or SD card.                                       *
//**************************************************************************************************
String selectnextFSnode ( int16_t delta )
{
  if ( usb_sd == FS_USB )                             // File system depends on this switch
  {
    return selectnextUSBnode ( delta ) ;              // Use USB
  }
  return selectnextSDnode ( delta ) ;                 // Use SD
}



//**************************************************************************************************
//                                C O N N E C T T O F I L E                                        *
//**************************************************************************************************
// Connect to USB or SD file.                                                                      *
//**************************************************************************************************
bool connecttofile()
{
  if ( usb_sd == FS_USB )                             // File system depends on this switch
  {
    return connecttofile_USB() ;                      // Use USB
  }
  return connecttofile_SD() ;                         // Use SD
}



//**************************************************************************************************
//                                            R E A D _ F S                                        *
//**************************************************************************************************
// Read from filesystem (USB or SD file).                                                          *
//**************************************************************************************************
int read_FS ( uint8_t* buf, uint32_t len )
{
  if ( usb_sd == FS_USB )                             // File system depends on this switch
  {
    return read_USBDRIVE ( buf, len ) ;               // Use USB
  }
  return read_SDCARD ( buf, len ) ;                   // Use SD
}


//**************************************************************************************************
//                                  L I S T F S T R A C K S                                        *
//**************************************************************************************************
// Read tracks from filesystem (USB or SD file).                                                   *
//**************************************************************************************************
int listfstracks ( const char* dirname, int level = 0, bool send = true )
{
  if ( usb_sd == FS_USB )                             // File system depends on this switch
  {
    return listusbtracks ( dirname, level, send ) ;   // Use USB
  }
  return listsdtracks ( dirname, level, send ) ;      // Use SD
}


//**************************************************************************************************
//                                G E T _ F S _ N O D E C O U N T                                  *
//**************************************************************************************************
// Return the number of nodes on USB/SD.                                                           *
//**************************************************************************************************
int get_FS_nodecount()
{
  if ( usb_sd == FS_USB )                             // File system depends on this switch
  {
    return USB_nodecount ;                            // Use USB
  }
  return SD_nodecount ;                               // Use SD
  
}


//**************************************************************************************************
//                                S H O W S T R E A M T I T L E                                    *
//**************************************************************************************************
// Show artist and songtitle if present in metadata.                                               *
// Show always if full=true.                                                                       *
//**************************************************************************************************
void showstreamtitle ( const char *ml, bool full )
{
  char*             p1 ;
  char*             p2 ;
  char              streamtitle[150] ;           // Streamtitle from metadata

  if ( strstr ( ml, "StreamTitle=" ) )
  {
    dbgprint ( "Streamtitle found, %d bytes", strlen ( ml ) ) ;
    dbgprint ( ml ) ;
    p1 = (char*)ml + 12 ;                       // Begin of artist and title
    if ( ( p2 = strstr ( ml, ";" ) ) )          // Search for end of title
    {
      if ( *p1 == '\'' )                        // Surrounded by quotes?
      {
        p1++ ;
        p2-- ;
      }
      *p2 = '\0' ;                              // Strip the rest of the line
    }
    // Save last part of string as streamtitle.  Protect against buffer overflow
    strncpy ( streamtitle, p1, sizeof ( streamtitle ) ) ;
    streamtitle[sizeof ( streamtitle ) - 1] = '\0' ;
  }
  else if ( full )
  {
    // Info probably from playlist
    strncpy ( streamtitle, ml, sizeof ( streamtitle ) ) ;
    streamtitle[sizeof ( streamtitle ) - 1] = '\0' ;
  }
  else
  {
    icystreamtitle = "" ;                       // Unknown type
    return ;                                    // Do not show
  }
  // Save for status request from browser and for MQTT
  icystreamtitle = streamtitle ;
  if ( ( p1 = strstr ( streamtitle, " - " ) ) ) // look for artist/title separator
  {
    p2 = p1 + 3 ;                               // 2nd part of text at this position
    if ( displaytype == T_NEXTION )
    {
      *p1++ = '\\' ;                            // Found: replace 3 characters by "\r"
      *p1++ = 'r' ;                             // Found: replace 3 characters by "\r"
    }
    else
    {
      *p1++ = '\n' ;                            // Found: replace 3 characters by newline
    }
    if ( *p2 == ' ' )                           // Leading space in title?
    {
      p2++ ;
    }
    strcpy ( p1, p2 ) ;                         // Shift 2nd part of title 2 or 3 places
  }
  tftset ( 1, streamtitle ) ;                   // Set screen segment text middle part
}


//**************************************************************************************************
//                                    S E T D A T A M O D E                                        *
//**************************************************************************************************
// Change the datamode and show in debug for testing.                                              *
//**************************************************************************************************
void setdatamode ( datamode_t newmode )
{
  //dbgprint ( "Change datamode from 0x%03X to 0x%03X",
  //           (int)datamode, (int)newmode ) ;
  datamode = newmode ;
}


//**************************************************************************************************
//                                    S T O P _ M P 3 C L I E N T                                  *
//**************************************************************************************************
// Disconnect from the server.                                                                     *
//**************************************************************************************************
void stop_mp3client ()
{

  while ( mp3client.connected() )
  {
    dbgprint ( "Stopping client" ) ;               // Stop connection to host
    mp3client.flush() ;
    mp3client.stop() ;
    delay ( 500 ) ;
  }
  mp3client.flush() ;                              // Flush stream client
  mp3client.stop() ;                               // Stop stream client
}


//**************************************************************************************************
//                                    C O N N E C T T O H O S T                                    *
//**************************************************************************************************
// Connect to the Internet radio server specified by newpreset.                                    *
//**************************************************************************************************
bool connecttohost()
{
  int         inx ;                                 // Position of ":" in hostname
  uint16_t    port = 80 ;                           // Port number for host
  String      extension = "/" ;                     // May be like "/mp3" in "skonto.ls.lv:8002/mp3"
  String      hostwoext = host ;                    // Host without extension and portnumber
  String      auth  ;                               // For basic authentication

  stop_mp3client() ;                                // Disconnect if still connected
  dbgprint ( "Connect to new host %s", host.c_str() ) ;
  tftset ( 0, "ESP32-Radio" ) ;                     // Set screen segment text top line
  tftset ( 1, "" ) ;                                // Clear song and artist
  displaytime ( "" ) ;                              // Clear time on TFT screen
  setdatamode ( INIT ) ;                            // Start default in metamode
  chunked = false ;                                 // Assume not chunked
  if ( host.endsWith ( ".m3u" ) )                   // Is it an m3u playlist?
  {
    playlist = host ;                               // Save copy of playlist URL
    setdatamode ( PLAYLISTINIT ) ;                  // Yes, start in PLAYLIST mode
    if ( playlist_num == 0 )                        // First entry to play?
    {
      playlist_num = 1 ;                            // Yes, set index
    }
    dbgprint ( "Playlist request, entry %d", playlist_num ) ;
  }
  // In the URL there may be an extension, like noisefm.ru:8000/play.m3u&t=.m3u
  inx = host.indexOf ( "/" ) ;                      // Search for begin of extension
  if ( inx > 0 )                                    // Is there an extension?
  {
    extension = host.substring ( inx ) ;            // Yes, change the default
    hostwoext = host.substring ( 0, inx ) ;         // Host without extension
  }
  // In the host there may be a portnumber
  inx = hostwoext.indexOf ( ":" ) ;                 // Search for separator
  if ( inx >= 0 )                                   // Portnumber available?
  {
    port = host.substring ( inx + 1 ).toInt() ;     // Get portnumber as integer
    hostwoext = host.substring ( 0, inx ) ;         // Host without portnumber
  }
  dbgprint ( "Connect to %s on port %d, extension %s",
             hostwoext.c_str(), port, extension.c_str() ) ;
  if ( mp3client.connect ( hostwoext.c_str(), port ) )
  {
    dbgprint ( "Connected to server" ) ;
    if ( nvssearch ( "basicauth" ) )                // Does "basicauth" exists?
    {
      auth = nvsgetstr ( "basicauth" ) ;            // Use basic authentication?
      if ( auth != "" )                             // Should be user:passwd
      { 
         auth = base64::encode ( auth.c_str() ) ;   // Encode
         auth = String ( "Authorization: Basic " ) +
                auth + String ( "\r\n" ) ;
      }
    }
    mp3client.print ( String ( "GET " ) +
                      extension +
                      String ( " HTTP/1.1\r\n" ) +
                      String ( "Host: " ) +
                      hostwoext +
                      String ( "\r\n" ) +
                      String ( "Icy-MetaData:1\r\n" ) +
                      auth +
                      String ( "Connection: close\r\n\r\n" ) ) ;
    return true ;
  }
  dbgprint ( "Request %s failed!", host.c_str() ) ;
  return false ;
}


//**************************************************************************************************
//                                      S S C O N V                                                *
//**************************************************************************************************
// Convert an array with 4 "synchsafe integers" to a number.                                       *
// There are 7 bits used per byte.                                                                 *
//**************************************************************************************************
uint32_t ssconv ( const uint8_t* bytes )
{
  uint32_t res = 0 ;                                      // Result of conversion
  uint8_t  i ;                                            // Counter number of bytes to convert

  for ( i = 0 ; i < 4 ; i++ )                             // Handle 4 bytes
  {
    res = res * 128 + bytes[i] ;                          // Convert next 7 bits
  }
  return res ;                                            // Return the result
}


#if (ETHERNET==1)


//**************************************************************************************************
//                                       C O N N E C T E T H                                       *
//**************************************************************************************************
// Connecting to ETHERNET.                                                                         *
// This function waits for a defined period (should be at least 5 seconds in my experience) for    *
// ethernet ip                                                                                     *
// Returns true if we got IP from ethernet within time.                                            *
// Problem with ethernet-stack: once started, it can not be stopped again (i. e. if not plugged in)*
// As a result of running still in background, WiFi will become to slow. Therefore (if not connect *
// possible within set timeout), a flag is set in NVS and a reset is forced. This flag will be     *
// evaluated and if set no connect attempt is made but should be left to WiFi.                     *
//**************************************************************************************************
bool connecteth() {
uint32_t myStartTime = millis();
uint32_t ethTimeout = ETHERNET_CONNECT_TIMEOUT * 1000;
  if (nvssearch("eth_timeout"))                                   // Ethernet timeout in preferences?
  {
    String val = String(nvsgetstr ("eth_timeout"));               // De-reference to another NVS-Entry?
#if defined(RETRORADIO)
    chomp_nvs(val);                                               // remove anything unnecessary
#else
    chomp(val);
#endif
    if (val == "0")                                               // Explicitely set to Zero?
    {
      dbgprint ( "Ethernet disabled in preferences "              // Tell it
                 "(""eth_timeout = 0"" found)" );
      return false;                                               // And do not not attempt
    }
    ethTimeout = val.toInt();                                     // convert to integer
    if (ethTimeout < ETHERNET_CONNECT_TIMEOUT)
      ethTimeout = ETHERNET_CONNECT_TIMEOUT; 
    else if (ethTimeout > 2 * ETHERNET_CONNECT_TIMEOUT)
    {
      dbgprint ("eth_timeout set to %d (seconds) in preferences. This will cause a long delay if ethernet fails/is not connected!", ethTimeout);
    }
    ethTimeout *= 1000;
  }
  WiFi.disconnect(true);                                          // Make sure old connections are stopped
  myStartTime = millis();
  WiFi.onEvent(ethevent);                                         // Event handler to catch connect status
  if (ETH.begin(ini_block.eth_phy_addr, ini_block.eth_phy_power, ini_block.eth_phy_mdc, 
                ini_block.eth_phy_mdio, 
                (eth_phy_type_t)ini_block.eth_phy_type, 
                (eth_clock_mode_t)ini_block.eth_clk_mode))
    while (!EthernetFound && (millis() - myStartTime < ethTimeout))
      delay(5);                                                     // wait for ethernet to succeed (or timeout)
  WiFi.removeEvent(ethevent);                                     // event handler not needed anymore
  if (!EthernetFound) {                                           // connection failed?
      dbgprint("Ethernet did not work! Will try WiFi next!");   
      esp_eth_deinit();                                           // leaving Ethernet stack on would lead to 
                                                                  // distortions on WiFi
  }
  return EthernetFound;
}


//**************************************************************************************************
//                                         E T H E V E N T                                         *
//**************************************************************************************************
// Callback for Ethernet related system events. Main purpose is to set EthernetFound and           *
// NetworkFound flags.                                                                             *
//**************************************************************************************************

void ethevent(WiFiEvent_t event)
{
  char *pfs;
  switch (event) {
    case SYSTEM_EVENT_ETH_START:
      dbgprint("ETH Started");
      //set eth hostname here
      ETH.setHostname(NAME);
      break;
    case SYSTEM_EVENT_ETH_CONNECTED:
      dbgprint("ETH Connected");
      break;
    case SYSTEM_EVENT_ETH_GOT_IP:
      ipaddress = ETH.localIP().toString() ;             // Form IP address
      pfs = dbgprint ( "Connected to Ethernet");
      tftlog ( pfs ) ;
      pfs = dbgprint ( "IP = %s", ipaddress.c_str() ) ;   // String to dispay on TFT
      tftlog ( pfs ) ;
      EthernetFound = true;
      break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
      pfs = dbgprint("ETH Disconnected");
      tftlog ( pfs );
      if (EthernetFound) {
        EthernetFound = false;
        NetworkFound = false;
      }
      break;
    case SYSTEM_EVENT_ETH_STOP:
      pfs = dbgprint("ETH Stopped");
      tftlog ( pfs );
      if (EthernetFound) {
        EthernetFound = false;
        NetworkFound = false;
      }
      break;
    default:
      break;
  }
}

#endif

//**************************************************************************************************
//                                       C O N N E C T W I F I                                     *
//**************************************************************************************************
// Connect to WiFi using the SSID's available in wifiMulti.                                        *
// If only one AP if found in preferences (i.e. wifi_00) the connection is made without            *
// using wifiMulti.                                                                                *
// If connection fails, an AP is created and the function returns false.                           *
//**************************************************************************************************
bool connectwifi()
{
  char*      pfs ;                                      // Pointer to formatted string
  char*      pfs2 ;                                     // Pointer to formatted string
  bool       localAP = false ;                          // True if only local AP is left

  WifiInfo_t winfo ;                                    // Entry from wifilist

  WiFi.disconnect(true) ;                               // After restart the router could
  WiFi.softAPdisconnect(true) ;                         // still keep the old connection
  if ( wifilist.size()  )                               // Any AP defined?
  {
    if ( wifilist.size() == 1 )                         // Just one AP defined in preferences?
    {
      winfo = wifilist[0] ;                             // Get this entry
      WiFi.begin ( winfo.ssid, winfo.passphrase ) ;     // Connect to single SSID found in wifi_xx
      dbgprint ( "Try WiFi %s", winfo.ssid ) ;          // Message to show during WiFi connect
    }
    else                                                // More AP to try
    {
      wifiMulti.run() ;                                 // Connect to best network
    }
    if (  WiFi.waitForConnectResult() != WL_CONNECTED ) // Try to connect
    {
#if defined(RETRORADIO)
      dbgprint("Wifi connect failed, try once again...");
      WiFi.disconnect(true);
      WiFi.softAPdisconnect(true);
      if ( wifilist.size() == 1 )                         // Just one AP defined in preferences?
        WiFi.begin ( winfo.ssid, winfo.passphrase ) ;     // Connect to single SSID found in wifi_xx
      else                                                // More AP to try
        wifiMulti.run() ;                                 // Connect to best network
      if (  WiFi.waitForConnectResult() != WL_CONNECTED ) // Try to connect      
#endif
      localAP = true ;                                  // Error, setup own AP
    }
  }
  else
  {
    localAP = true ;                                    // Not even a single AP defined
  }
  if ( localAP )                                        // Must setup local AP?
  {
    dbgprint ( "WiFi Failed!  Trying to setup AP with name %s and password %s.", NAME, NAME ) ;
    WiFi.softAP ( NAME, NAME ) ;                        // This ESP will be an AP
    pfs = dbgprint ( "IP = 192.168.4.1" ) ;             // Address for AP
  }
  else
  {
    ipaddress = WiFi.localIP().toString() ;             // Form IP address
    pfs2 = dbgprint ( "Connected to %s", WiFi.SSID().c_str() ) ;
    tftlog ( pfs2 ) ;
    pfs = dbgprint ( "IP = %s", ipaddress.c_str() ) ;   // String to dispay on TFT
  }
  tftlog ( pfs ) ;                                      // Show IP
  delay ( 3000 ) ;                                      // Allow user to read this
  tftlog ( "\f" ) ;                                     // Select new page if NEXTION 
  return ( localAP == false ) ;                         // Return result of connection
}


//**************************************************************************************************
//                                           O T A S T A R T                                       *
//**************************************************************************************************
// Update via WiFi has been started by Arduino IDE or update request.                              *
//**************************************************************************************************
void otastart()
{
  char* p ;

  p = dbgprint ( "OTA update Started" ) ;
  tftset ( 2, p ) ;                                   // Set screen segment bottom part
}


//**************************************************************************************************
//                                D O _ N E X T I O N _ U P D A T E                                *
//**************************************************************************************************
// Update NEXTION image from OTA stream.                                                           *
//**************************************************************************************************
bool do_nextion_update ( uint32_t clength )
{
  bool     res = false ;                                       // Update result
  uint32_t k ;
  int      c ;                                                 // Reply from NEXTION

  if ( nxtserial )                                             // NEXTION active?
  {
    vTaskDelete ( xspftask ) ;                                 // Prevent output to NEXTION
    delay ( 1000 ) ;
    nxtserial->printf ( "\xFF\xFF\xFF" ) ;                     // Empty command
    for ( int i = 0 ; i < 100 ; i++ )                          // Any input seen?
    {
      if ( nxtserial->available() )
      {
        c =  nxtserial->read() ;                               // Read garbage
      }
      delay ( 20 ) ;
    }
    nxtserial->printf ( "whmi-wri %d,9600,0\xFF\xFF\xFF",      // Start upload
                        clength ) ;
    while ( !nxtserial->available() )                          // Any input seen?
    {
      delay ( 20 ) ;
    }
    c =  nxtserial->read() ;                                   // Yes, read the 0x05 ACK
    while ( clength )                                          // Loop for the transfer
    {
      k = clength ;
      if ( k > 4096 )
      {
        k = 4096 ;
      }
      k = otaclient.read ( tmpbuff, k ) ;                      // Read a number of bytes from the stream
      dbgprint ( "TFT file, read %d bytes", k ) ;
      nxtserial->write ( tmpbuff, k ) ;     
      while ( !nxtserial->available() )                        // Any input seen?
      {
        delay ( 20 ) ;
      }
      c =  (char)nxtserial->read() ;                           // Yes, read the 0x05 ACK
      if ( c != 0x05 )
      {
        break ;
      }
      clength -= k ;
    }
    otaclient.flush() ;
    if ( clength == 0 )
    {
      dbgprint ( "Update successfully completed" ) ;
      res = true ;
    }
  }
  return res ;
}


//**************************************************************************************************
//                                D O _ S O F T W A R E _ U P D A T E                              *
//**************************************************************************************************
// Update software from OTA stream.                                                                *
//**************************************************************************************************
bool do_software_update ( uint32_t clength )
{
  bool res = false ;                                          // Update result
  
  if ( Update.begin ( clength ) )                             // Update possible?
  {
    dbgprint ( "Begin OTA update, length is %d",
               clength ) ;
    if ( Update.writeStream ( otaclient ) == clength )        // writeStream is the real download
    {
      dbgprint ( "Written %d bytes successfully", clength ) ;
    }
    else
    {
      dbgprint ( "Write failed!" ) ;
    }
    if ( Update.end() )                                       // Check for successful flash
    {
      dbgprint( "OTA done" ) ;
      if ( Update.isFinished() )
      {
        dbgprint ( "Update successfully completed" ) ;
        res = true ;                                          // Positive result
      }
      else
      {
        dbgprint ( "Update not finished!" ) ;
      }
    }
    else
    {
      dbgprint ( "Error Occurred. Error %s", Update.getError() ) ;
    }
  }
  else
  {
    // Not enough space to begin OTA
    dbgprint ( "Not enough space to begin OTA" ) ;
    otaclient.flush() ;
  }
  return res ;
}


//**************************************************************************************************
//                                        U P D A T E _ S O F T W A R E                            *
//**************************************************************************************************
// Update software by download from remote host.                                                   *
//**************************************************************************************************
void update_software ( const char* lstmodkey, const char* updatehost, const char* binfile )
{
  uint32_t    timeout = millis() ;                              // To detect time-out
  String      line ;                                            // Input header line
  String      lstmod = "" ;                                     // Last modified timestamp in NVS
  String      newlstmod ;                                       // Last modified from host
  
  updatereq = false ;                                           // Clear update flag
  otastart() ;                                                  // Show something on screen
  stop_mp3client () ;                                           // Stop input stream
  lstmod = nvsgetstr ( lstmodkey ) ;                            // Get current last modified timestamp
  dbgprint ( "Connecting to %s for %s",
              updatehost, binfile ) ;
  if ( !otaclient.connect ( updatehost, 80 ) )                  // Connect to host
  {
    dbgprint ( "Connect to updatehost failed!" ) ;
    return ;
  }
  otaclient.printf ( "GET %s HTTP/1.1\r\n"
                     "Host: %s\r\n"
                     "Cache-Control: no-cache\r\n"
                     "Connection: close\r\n\r\n",
                     binfile,
                     updatehost ) ;
  while ( otaclient.available() == 0 )                          // Wait until response appears
  {
    if ( millis() - timeout > 5000 )
    {
      dbgprint ( "Connect to Update host Timeout!" ) ;
      otaclient.stop() ;
      return ;
    }
  }
  // Connected, handle response
  while ( otaclient.available() )
  {
    line = otaclient.readStringUntil ( '\n' ) ;                 // Read a line from response
    line.trim() ;                                               // Remove garbage
    dbgprint ( line.c_str() ) ;                                 // Debug info
    if ( !line.length() )                                       // End of headers?
    {
      break ;                                                   // Yes, get the OTA started
    }
    // Check if the HTTP Response is 200.  Any other response is an error.
    if ( line.startsWith ( "HTTP/1.1" ) )                       // 
    {
      if ( line.indexOf ( " 200 " ) < 0 )
      {
        dbgprint ( "Got a non 200 status code from server!" ) ;
        return ;
      }
    }
    scan_content_length ( line.c_str() ) ;                      // Scan for content_length
    if ( line.startsWith ( "Last-Modified: " ) )                // Timestamp of binary file
    {
      newlstmod = line.substring ( 15 ) ;                       // Isolate timestamp
    }
  }
  // End of headers reached
  if ( newlstmod == lstmod )                                    // Need for update?
  {
    dbgprint ( "No new version available" ) ;                   // No, show reason
    otaclient.flush() ;
    return ;    
  }
  if ( clength > 0 )
  {
    if ( strstr ( binfile, ".bin" ) )                           // Update of the sketch?
    {
      if ( do_software_update ( clength ) )                     // Flash updated sketch
      {
        nvssetstr ( lstmodkey, newlstmod ) ;                    // Update Last Modified in NVS
      }
    }
    if ( strstr ( binfile, ".tft" ) )                           // Update of the NEXTION image?
    {
      if ( do_nextion_update ( clength ) )                      // Flash updated NEXTION
      {
        nvssetstr ( lstmodkey, newlstmod ) ;                    // Update Last Modified in NVS
      }
    }
  }
  else
  {
    dbgprint ( "There was no content in the response" ) ;
    otaclient.flush() ;
  }
}


//**************************************************************************************************
//                                  R E A D H O S T F R O M P R E F                                *
//**************************************************************************************************
// Read the mp3 host from the preferences specified by the parameter.                              *
// The host will be returned.                                                                      *
// We search for "preset_x" or "preset_xx" or "preset_xxx".                       *
//**************************************************************************************************
String readhostfrompref ( int16_t preset )
{
  char           tkey[12] ;                            // Key as an array of char

  sprintf ( tkey, "preset_%d", preset ) ;              // Form the search key
  if ( !nvssearch ( tkey ) )                           // Does _x[x[x]] exists?
  {
    sprintf ( tkey, "preset_%03d", preset ) ;          // Form new search key
    if ( !nvssearch ( tkey ) )                         // Does _xxx exists?
    {
      sprintf ( tkey, "preset_%02d", preset ) ;        // Form new search key
    }
    if ( !nvssearch ( tkey ) )                         // Does _xx exists?
    {
      return String ( "" ) ;                           // Not found
    }
  }
  // Get the contents
  return nvsgetstr ( tkey ) ;                          // Get the station (or empty sring)
}


//**************************************************************************************************
//                                  R E A D H O S T F R O M P R E F                                *
//**************************************************************************************************
// Search for the next mp3 host in preferences specified newpreset.                                *
// The host will be returned.  newpreset will be updated                                           *
//**************************************************************************************************
String readhostfrompref()
{
  String contents = "" ;                                // Result of search
  int    maxtry = 0 ;                                   // Limit number of tries

  while ( ( contents = readhostfrompref ( ini_block.newpreset ) ) == "" )
  {
    if ( ++maxtry >= MAXPRESETS )
    {
      return "" ;
    }
    if ( ++ini_block.newpreset >= MAXPRESETS )          // Next or wrap to 0
    {
      ini_block.newpreset = 0 ;
    }
  }
  // Get the contents
  return contents ;                                     // Return the station
}


//**************************************************************************************************
//                                       R E A D P R O G B U T T O N S                             *
//**************************************************************************************************
// Read the preferences for the programmable input pins and the touch pins.                        *
//**************************************************************************************************
void readprogbuttons()
{
#if !defined(RETRORADIO)  
  char        mykey[20] ;                                   // For numerated key
  int8_t      pinnr ;                                       // GPIO pinnumber to fill
  int         i ;                                           // Loop control
  String      val ;                                         // Contents of preference entry

  for ( i = 0 ; ( pinnr = progpin[i].gpio ) >= 0 ; i++ )    // Scan for all programmable pins
  {
    sprintf ( mykey, "gpio_%02d", pinnr ) ;                 // Form key in preferences
    if ( nvssearch ( mykey ) )
    {
      val = nvsgetstr ( mykey ) ;                           // Get the contents
      if ( val.length() )                                   // Does it exists?
      {
        if ( !progpin[i].reserved )                         // Do not use reserved pins
        {
          progpin[i].avail = true ;                         // This one is active now
          progpin[i].command = val ;                        // Set command
          dbgprint ( "gpio_%02d will execute %s",           // Show result
                     pinnr, val.c_str() ) ;
        }
      }
    }
  }
  // Now for the touch pins 0..9, identified by their GPIO pin number
  for ( i = 0 ; ( pinnr = touchpin[i].gpio ) >= 0 ; i++ )   // Scan for all programmable pins
  {
    sprintf ( mykey, "touch_%02d", i ) ;                    // Form key in preferences
    if ( nvssearch ( mykey ) )
    {
      val = nvsgetstr ( mykey ) ;                           // Get the contents
      if ( val.length() )                                   // Does it exists?
      {
        if ( !touchpin[i].reserved )                        // Do not use reserved pins
        {
          touchpin[i].avail = true ;                        // This one is active now
          touchpin[i].command = val ;                       // Set command
          //pinMode ( touchpin[i].gpio,  INPUT ) ;          // Free floating input
          dbgprint ( "touch_%02d will execute %s",          // Show result
                     i, val.c_str() ) ;
          dbgprint ( "Level is now %d",
                     touchRead ( pinnr ) ) ;                // Sample the pin
        }
        else
        {
          dbgprint ( "touch_%02d pin (GPIO%02d) is reserved for I/O!",
                     i, pinnr ) ;
        }
      }
    }
  }
#endif
}


//**************************************************************************************************
//                                       R E S E R V E P I N                                       *
//**************************************************************************************************
// Set I/O pin to "reserved".                                                                      *
// The pin is than not available for a programmable function.                                      *
//**************************************************************************************************
void reservepin ( int8_t rpinnr )
{
  uint8_t i = 0 ;                                           // Index in progpin/touchpin array
  int8_t  pin ;                                             // Pin number in progpin array

  while ( ( pin = progpin[i].gpio ) >= 0 )                  // Find entry for requested pin
  {
    if ( pin == rpinnr )                                    // Entry found?
    {
      if ( progpin[i].reserved )                            // Already reserved?
      {
        dbgprint ( "Pin %d is already reserved!", rpinnr ) ;
      }
      //dbgprint ( "GPIO%02d unavailabe for 'gpio_'-command", pin ) ;
      progpin[i].reserved = true ;                          // Yes, pin is reserved now
      break ;                                               // No need to continue
    }
    i++ ;                                                   // Next entry
  }
  // Also reserve touchpin numbers
  i = 0 ;
  while ( ( pin = touchpin[i].gpio ) >= 0 )                 // Find entry for requested pin
  {
    if ( pin == rpinnr )                                    // Entry found?
    {
      //dbgprint ( "GPIO%02d unavailabe for 'touch'-command", pin ) ;
      touchpin[i].reserved = true ;                         // Yes, pin is reserved now
      break ;                                               // No need to continue
    }
    i++ ;                                                   // Next entry
  }
}


//**************************************************************************************************
//                                       R E A D I O P R E F S                                     *
//**************************************************************************************************
// Scan the preferences for IO-pin definitions.                                                    *
//**************************************************************************************************
void readIOprefs()
{
  struct iosetting
  {
    const char* gname ;                                   // Name in preferences
    int*        gnr ;                                     // GPIO pin number
    int         pdefault ;                                // Default pin
  };
  struct iosetting klist[] = {                            // List of I/O related keys
    { "pin_ir",        &ini_block.ir_pin,           -1 },
    { "pin_ir_nec",    &ini_block.ir_pin_nec,       -1 },
    { "pin_ir_rc5",    &ini_block.ir_pin_rc5,       -1 },
    { "pin_enc_clk",   &ini_block.enc_clk_pin,      -1 },
    { "pin_enc_dt",    &ini_block.enc_dt_pin,       -1 },
    { "pin_enc_sw",    &ini_block.enc_sw_pin,       -1 },
    { "pin_tft_cs",    &ini_block.tft_cs_pin,       -1 }, // Display SPI version
    { "pin_tft_dc",    &ini_block.tft_dc_pin,       -1 }, // Display SPI version
    { "pin_tft_scl",   &ini_block.tft_scl_pin,      -1 }, // Display I2C version
    { "pin_tft_sda",   &ini_block.tft_sda_pin,      -1 }, // Display I2C version
    { "pin_tft_bl",    &ini_block.tft_bl_pin,       -1 }, // Display backlight
    { "pin_tft_blx",   &ini_block.tft_blx_pin,      -1 }, // Display backlight (inversed logic)
    { "pin_sd_cs",     &ini_block.sd_cs_pin,        -1 },
    { "pin_ch376_cs",  &ini_block.ch376_cs_pin,     -1 }, // CH376 CS for USB interface
    { "pin_ch376_int", &ini_block.ch376_int_pin,    -1 }, // CH376 INT for USB interfce
    { "pin_vs_cs",     &ini_block.vs_cs_pin,        -1 }, // VS1053 pins
    { "pin_vs_dcs",    &ini_block.vs_dcs_pin,       -1 },
    { "pin_vs_dreq",   &ini_block.vs_dreq_pin,      -1 },
    { "pin_shutdown",  &ini_block.vs_shutdown_pin,  -1 }, // Amplifier shut-down pin
    { "pin_shutdownx", &ini_block.vs_shutdownx_pin, -1 }, // Amplifier shut-down pin (inversed logic)
    { "pin_spi_sck",   &ini_block.spi_sck_pin,      SCK },
    { "pin_spi_miso",  &ini_block.spi_miso_pin,     MISO },
    { "pin_spi_mosi",  &ini_block.spi_mosi_pin,     MOSI },
#if (ETHERNET == 1)
    { "_noreserve_",   NULL,                        -2 },
    { "eth_phy_addr",  &ini_block.eth_phy_addr,     ETH_PHY_ADDR },    // Ethernet Phy Address setting
    { "eth_phy_power", &ini_block.eth_phy_power,    ETH_PHY_POWER },   // Ethernet Power setting
    { "eth_phy_mdc",   &ini_block.eth_phy_mdc,      ETH_PHY_MDC },     // Ethernet MDC setting
    { "eth_phy_mdio",  &ini_block.eth_phy_mdio,     ETH_PHY_MDIO },    // Ethernet Phy MDIO setting
    { "eth_phy_type",  &ini_block.eth_phy_type,     ETH_PHY_TYPE },    // Ethernet Phy Type setting
    { "eth_clk_mode",  &ini_block.eth_clk_mode,     ETH_CLK_MODE },    // Ethernet clock mode setting
#endif

/*
#if defined(RETRORADIO)
    { "_noreserve_",   NULL,                        -2 },
    { "pin_rr_led0",   &ini_block.retr_led0_pin,     -1}, // GPIO connected to Retro Radio LED
    { "pin_rr_led1",   &ini_block.retr_led1_pin,     -1}, // GPIO connected to Retro Radio LED
    { "pin_rr_led2",   &ini_block.retr_led2_pin,     -1}, // GPIO connected to Retro Radio LED
    { "pin_rr_led3",   &ini_block.retr_led3_pin,     -1}, // GPIO connected to Retro Radio LED
    { "pin_rr_led4",   &ini_block.retr_led4_pin,     -1}, // GPIO connected to Retro Radio LED
    { "pin_rr_led5",   &ini_block.retr_led5_pin,     -1}, // GPIO connected to Retro Radio LED
    { "pin_rr_led6",   &ini_block.retr_led6_pin,     -1}, // GPIO connected to Retro Radio LED
    { "pin_rr_led7",   &ini_block.retr_led7_pin,     -1}, // GPIO connected to Retro Radio LED
    { "pin_rr_led8",   &ini_block.retr_led8_pin,     -1}, // GPIO connected to Retro Radio LED
    { "pin_rr_led9",   &ini_block.retr_led9_pin,     -1}, // GPIO connected to Retro Radio LED
    { "pin_rr_vol",    &ini_block.retr_vol_pin,      -1}, // GPIO connected to Retro Radio Volume pin
    { "pin_rr_switch", &ini_block.retr_switch_pin, -1}, // GPIO connected to Retro Radio WaveBandSwitch
    { "pin_rr_tune",   &ini_block.retr_tune_pin,     -1}, // GPIO connected to Retro Radio tuning capacitor (must be touch pin!)
    { "pin_rr_touch",  &ini_block.retr_touch_pin,    -1}, // GPIO connected to Retro Radio touch area (must be touch pin!)
#endif
*/
    { NULL,            NULL,                        0  }  // End of list
  } ;
  int         i ;                                         // Loop control
  int         count = 0 ;                                 // Number of keys found
  String      val ;                                       // Contents of preference entry
  int         ival ;                                      // Value converted to integer
  int*        p ;                                         // Points to variable
  bool        noreserve = false ;                         // After *gnr == NULL found, do no longer reserve 
                                                          // pins (but do ini_block setting still) 
  for ( i = 0 ; klist[i].gname ; i++ )                    // Loop trough all I/O related keys
  {
    p = klist[i].gnr ;                                    // Point to target variable
    if (!p)
      noreserve = true ;
    else
    {
      ival = klist[i].pdefault ;                          // Assume pin number to be the default
      if ( nvssearch ( klist[i].gname ) )                 // Does it exist?
      {
        val = nvsgetstr ( klist[i].gname ) ;              // Read value of key
        if ( val.length() )                               // Parameter in preference?
        {
          count++ ;                                       // Yes, count number of filled keys
          ival = val.toInt() ;                            // Convert value to integer pinnumber
          if (!noreserve)
            reservepin ( ival ) ;                         // Set pin to "reserved"
        }
      }
      *p = ival ;                                         // Set pinnumber in ini_block
      dbgprint ( "%s set to %d",                          // Show result
                 klist[i].gname,
                 ival ) ;
    }
  }
}


//**************************************************************************************************
//                                       R E A D P R E F S                                         *
//**************************************************************************************************
// Read the preferences and interpret the commands.                                                *
// If output == true, the key / value pairs are returned to the caller as a String.                *
//**************************************************************************************************
String readprefs ( bool output )
{
  uint16_t    i ;                                           // Loop control
  String      val ;                                         // Contents of preference entry
  String      cmd ;                                         // Command for analyzCmd
  String      outstr = "" ;                                 // Outputstring
  char*       key ;                                         // Point to nvskeys[i]
  uint8_t     winx ;                                        // Index in wifilist
  uint16_t    last2char = 0 ;                               // To detect paragraphs

  i = 0 ;
  while ( *( key = nvskeys[i] ) )                           // Loop trough all available keys
  {
    val = nvsgetstr ( key ) ;                               // Read value of this key
    cmd = String ( key ) +                                  // Yes, form command
          String ( " = " ) +
          val ;
    if ( strstr ( key, "wifi_"  ) )                         // Is it a wifi ssid/password?
    {
   //   Serial.println("WIFIWIFIWIFWIFIWIFIWIFIWIFIWIFIWIFIWIFIWIFIWIFIWIFIWIFIWIFIWIFI");
      winx = atoi ( key + 5 ) ;                             // Get index in wifilist
      if ( ( winx < wifilist.size() ) &&                    // Existing wifi spec in wifilist?
           ( val.indexOf ( wifilist[winx].ssid ) == 0 ) )
      {
        val = String ( wifilist[winx].ssid ) +              // Yes, hide password
              String ( "/*******" ) ;
      }
      cmd = String ( "" ) ;                                 // Do not analyze this
      
    }
    else if ( strstr ( key, "mqttpasswd"  ) )               // Is it a MQTT password?
    {
      val = String ( "*******" ) ;                          // Yes, hide it
    }
    if ( output )
    {
      if ( ( i > 0 ) &&
           ( *(uint16_t*)key != last2char ) )               // New paragraph?
      {
        outstr += String ( "#\n" ) ;                        // Yes, add separator
      }
      last2char = *(uint16_t*)key ;                         // Save 2 chars for next compare
      outstr += String ( key ) +                            // Add to outstr
                String ( " = " ) +
                val +
                String ( "\n" ) ;                           // Add newline
    }
    else
    {
      analyzeCmd ( cmd.c_str() ) ;                          // Analyze it
    }
    i++ ;                                                   // Next key
  }
  if ( i == 0 )
  {
    outstr = String ( "No preferences found.\n"
                      "Use defaults or run Esp32_radio_init first.\n" ) ;
  }
  return outstr ;
}


//**************************************************************************************************
//                                    M Q T T R E C O N N E C T                                    *
//**************************************************************************************************
// Reconnect to broker.                                                                            *
//**************************************************************************************************
bool mqttreconnect()
{
  static uint32_t retrytime = 0 ;                         // Limit reconnect interval
  bool            res = false ;                           // Connect result
  char            clientid[20] ;                          // Client ID
  char            subtopic[60] ;                          // Topic to subscribe

  if ( ( millis() - retrytime ) < 5000 )                  // Don't try to frequently
  {
    return res ;
  }
  retrytime = millis() ;                                  // Set time of last try
  if ( mqttcount > MAXMQTTCONNECTS )                      // Tried too much?
  {
    mqtt_on = false ;                                     // Yes, switch off forever
    return res ;                                          // and quit
  }
  mqttcount++ ;                                           // Count the retries
  dbgprint ( "(Re)connecting number %d to MQTT %s",       // Show some debug info
             mqttcount,
             ini_block.mqttbroker.c_str() ) ;
  sprintf ( clientid, "%s-%04d",                          // Generate client ID
            NAME, (int) random ( 10000 ) % 10000 ) ;
  res = mqttclient.connect ( clientid,                    // Connect to broker
                             ini_block.mqttuser.c_str(),
                             ini_block.mqttpasswd.c_str()
                           ) ;
  if ( res )
  {
    sprintf ( subtopic, "%s/%s",                          // Add prefix to subtopic
              ini_block.mqttprefix.c_str(),
              MQTT_SUBTOPIC ) ;
    res = mqttclient.subscribe ( subtopic ) ;             // Subscribe to MQTT
    if ( !res )
    {
      dbgprint ( "MQTT subscribe failed!" ) ;             // Failure
    }
    mqttpub.trigger ( MQTT_IP ) ;                         // Publish own IP
  }
  else
  {
    dbgprint ( "MQTT connection failed, rc=%d",
               mqttclient.state() ) ;

  }
  return res ;
}


//**************************************************************************************************
//                                    O N M Q T T M E S S A G E                                    *
//**************************************************************************************************
// Executed when a subscribed message is received.                                                 *
// Note that message is not delimited by a '\0'.                                                   *
// Note that cmd buffer is shared with serial input.                                               *
//**************************************************************************************************
void onMqttMessage ( char* topic, byte* payload, unsigned int len )
{
  const char*  reply ;                                // Result from analyzeCmd

  if ( strstr ( topic, MQTT_SUBTOPIC ) )              // Check on topic, maybe unnecessary
  {
    if ( len >= sizeof(cmd) )                         // Message may not be too long
    {
      len = sizeof(cmd) - 1 ;
    }
    strncpy ( cmd, (char*)payload, len ) ;            // Make copy of message
    cmd[len] = '\0' ;                                 // Take care of delimeter
    dbgprint ( "MQTT message arrived [%s], lenght = %d, %s", topic, len, cmd ) ;
    reply = analyzeCmd ( cmd ) ;                      // Analyze command and handle it
    dbgprint ( reply ) ;                              // Result for debugging
  }
}


//**************************************************************************************************
//                                     S C A N S E R I A L                                         *
//**************************************************************************************************
// Listen to commands on the Serial inputline.                                                     *
//**************************************************************************************************
void scanserial()
{
  static String serialcmd ;                      // Command from Serial input
  char          c ;                              // Input character
  const char*   reply = "" ;                     // Reply string from analyzeCmd
  uint16_t      len ;                            // Length of input string

  while ( Serial.available() )                   // Any input seen?
  {
    c =  (char)Serial.read() ;                   // Yes, read the next input character
    //Serial.write ( c ) ;                       // Echo
    len = serialcmd.length() ;                   // Get the length of the current string
    if ( ( c == '\n' ) || ( c == '\r' ) )
    {
      if ( len )
      {
        strncpy ( cmd, serialcmd.c_str(), sizeof(cmd) ) ;
        if ( nxtserial )                         // NEXTION test possible?
        {
          if ( serialcmd.startsWith ( "N:" ) )   // Command meant to test Nextion display?
          {
            nxtserial->printf ( "%s\xFF\xFF\xFF", cmd + 2 ) ;
          }
        }
        reply = analyzeCmd ( cmd ) ;             // Analyze command and handle it
        dbgprint ( reply ) ;                     // Result for debugging
        serialcmd = "" ;                         // Prepare for new command
      }
    }
    if ( c >= ' ' )                              // Only accept useful characters
    {
      serialcmd += c ;                           // Add to the command
    }
    if ( len >= ( sizeof(cmd) - 2 )  )           // Check for excessive length
    {
      serialcmd = "" ;                           // Too long, reset
    }
  }
}


//**************************************************************************************************
//                                     S C A N S E R I A L 2                                       *
//**************************************************************************************************
// Listen to commands on the 2nd Serial inputline (NEXTION).                                       *
//**************************************************************************************************
void scanserial2()
{
  static String  serialcmd ;                       // Command from Serial input
  char           c ;                               // Input character
  const char*    reply = "" ;                      // Reply string from analyzeCmd
  uint16_t       len ;                             // Length of input string
  static uint8_t ffcount = 0 ;                     // Counter for 3 tmes "0xFF"

  if ( nxtserial )                                 // NEXTION active?
  {
    while ( nxtserial->available() )               // Yes, any input seen?
    {
      c =  (char)nxtserial->read() ;               // Yes, read the next input character
      len = serialcmd.length() ;                   // Get the length of the current string
      if ( c == 0xFF )                             // End of command?
      {
        if ( ++ffcount < 3 )                       // 3 times FF?
        {
          continue ;                               // No, continue to read
        }
        ffcount = 0 ;                              // For next command
        if ( len )
        {
          strncpy ( cmd, serialcmd.c_str(), sizeof(cmd) ) ;
          dbgprint ( "NEXTION command seen %02X %s",
                     cmd[0], cmd + 1 ) ;
          if ( cmd[0] == 0x70 )                    // Button pressed?
          { 
            reply = analyzeCmd ( cmd + 1 ) ;       // Analyze command and handle it
            dbgprint ( reply ) ;                   // Result for debugging
          }
          serialcmd = "" ;                         // Prepare for new command
        }
      }
      else if ( c >= ' ' )                         // Only accept useful characters
      {
        serialcmd += c ;                           // Add to the command
      }
      if ( len >= ( sizeof(cmd) - 2 )  )           // Check for excessive length
      {
        serialcmd = "" ;                           // Too long, reset
      }
    }
  }
}


//**************************************************************************************************
//                                     S C A N D I G I T A L                                       *
//**************************************************************************************************
// Scan digital inputs.                                                                            *
//**************************************************************************************************
void  scandigital()
{
#if !defined(RETRORADIO)
  static uint32_t oldmillis = 5000 ;                        // To compare with current time
  int             i ;                                       // Loop control
  int8_t          pinnr ;                                   // Pin number to check
  bool            level ;                                   // Input level
  const char*     reply ;                                   // Result of analyzeCmd
  int16_t         tlevel ;                                  // Level found by touch pin
  const int16_t   THRESHOLD = 30 ;                          // Threshold or touch pins

  if ( ( millis() - oldmillis ) < 100 )                     // Debounce
  {
    return ;
  }
  scanios++ ;                                               // TEST*TEST*TEST
  oldmillis = millis() ;                                    // 100 msec over
  for ( i = 0 ; ( pinnr = progpin[i].gpio ) >= 0 ; i++ )    // Scan all inputs
  {
    if ( !progpin[i].avail || progpin[i].reserved )         // Skip unused and reserved pins
    {
      continue ;
    }
    level = ( digitalRead ( pinnr ) == HIGH ) ;             // Sample the pin
    if ( level != progpin[i].cur )                          // Change seen?
    {
      progpin[i].cur = level ;                              // And the new level
      if ( !level )                                         // HIGH to LOW change?
      {
        dbgprint ( "GPIO_%02d is now LOW, execute %s",
                   pinnr, progpin[i].command.c_str() ) ;
        reply = analyzeCmd ( progpin[i].command.c_str() ) ; // Analyze command and handle it
        dbgprint ( reply ) ;                                // Result for debugging
      }
    }
  }
  // Now for the touch pins
  for ( i = 0 ; ( pinnr = touchpin[i].gpio ) >= 0 ; i++ )   // Scan all inputs
  {
    if ( !touchpin[i].avail || touchpin[i].reserved )       // Skip unused and reserved pins
    {
      continue ;
    }
    tlevel = ( touchRead ( pinnr ) ) ;                      // Sample the pin
    level = ( tlevel >= THRESHOLD ) ;                       // True if below threshold
    if ( level )                                            // Level HIGH?
    {
      touchpin[i].count = 0 ;                               // Reset count number of times
    }
    else
    {
      if ( ++touchpin[i].count < 3 )                        // Count number of times LOW
      {
        level = true ;                                      // Not long enough: handle as HIGH
      }
    }
    if ( level != touchpin[i].cur )                         // Change seen?
    {
      touchpin[i].cur = level ;                             // And the new level
      if ( !level )                                         // HIGH to LOW change?
      {
        dbgprint ( "TOUCH_%02d is now %d ( < %d ), execute %s",
                   pinnr, tlevel, THRESHOLD,
                   touchpin[i].command.c_str() ) ;
        reply = analyzeCmd ( touchpin[i].command.c_str() ); // Analyze command and handle it
        dbgprint ( reply ) ;                                // Result for debugging
      }
    }
  }
#endif
}

//**************************************************************************************************
//                             E X E C U T E _ I R _ C O M M A N D                                 *
//**************************************************************************************************
// Searches NVS-Keys for specific IR command(s) and executes command(s) if found                   *
// The keycode is taken from ir-data struct.                                                       *  
//    if type == "", ir_XXXX commands will be searched for (key pressed)                           *
//    if type == "r", in order ir_XXXXRY, ir_XXXXrY and ir_XXXXr commands will be searched for (in *
//    that order) and the commands stored for first found key will be executed (key hold)          *
//    if type == "x", irXXXXx commands will be searched for (key released)                         *
//    for any other type content, type is assumed to already contain the nvs-searchkey             *
//  Returns true if any commands have been found (and executed)                                    * 
//  If verbose is set to "true", will also report if no entry has been found in NVS                *
//**************************************************************************************************

bool execute_ir_command ( String type, bool verbose=true ) 
{
  char *protocol_names[] = {"", " (NEC)", " (RC5)"} ;           // Known IR protocol names
  bool ret = false;
  char mykey[20];
  bool done = false;                                            // If more than one possible key results, set to 
                                                                // true if one of those keys already "fired"
  strcpy(mykey, type.c_str());                                  // Default: type contains the key already
  if ( type == "" )                                             // If not
    sprintf ( mykey, "ir_%04X", ir.command );                   // Generate key for "key pressed"
  else if ( type == "x" )                                       // Or
    sprintf ( mykey, "ir_%04Xx", ir.exitcommand );              // Generate key for "key released"
  else if ( type == "r" ) {                                     // Or 
    done = true;                                                // Generate and already execute sequence of "repeat" events
    sprintf ( mykey, "ir_%04XR%d",                              // First check, if there is a cyclic command sequence defined
          ir.command, ir.repeat_counter );
    ret = execute_ir_command ( String(mykey) );                 // did we have a cyclic sequence?
    if (ret)                                                    // if so
    {
      ir.repeat_counter = 0;                                    // Reset repeat counter
    }
    else                                                        // no cyclic sequence found
    {
      sprintf ( mykey, "ir_%04Xr%d",                            // Do we have a "longpress" with precise current repeat counter?
              ir.command, ir.repeat_counter );
      ret = execute_ir_command ( String(mykey), false );        // try and execute
      if (!ret)                                                 // Longpress event key for this precise repeat counter found?
      {                                                         // If not  
        sprintf ( mykey, "ir_%04Xr", ir.command );              // Search for "unspecific" longpress event ir_XXXXr
        ret = execute_ir_command ( String(mykey), false );
      }
    }
  }
  if (! done )                                                  // Did we already (tried) some key(s) (can be from longpress only)
  {                                                             // If not, mykey now contains the NVS key to check
    ret = nvssearch ( mykey ) ;                     
    if (! ret )                                                 // Key not found?
    {
      if ( verbose )                                            // Show info if key not found?
        dbgprint ( "IR event %s%s received but not found in preferences.",
          mykey, protocol_names[ir.protocol] ) ;
    }
    else 
    {
      String val = nvsgetstr ( mykey ) ;                    // Get the contents
      const char* reply;
#if defined(RETRORADIO)
      chomp_nvs ( val );
#else
      chomp ( val );
#endif

      dbgprint ( "IR code for %s%sreceived. Will execute %s",
               mykey, protocol_names[ir.protocol], val.c_str() ) ;
      reply = analyzeCmds ( val ) ;                         // Analyze command and handle it
      dbgprint ( reply ) ;                                  // Result for debugging
    }
  }
  return ret;
}

//**************************************************************************************************
//                                     S C A N I R                                                 *
//**************************************************************************************************
// See if IR input is available.  Execute the programmed command.                                  *
//**************************************************************************************************
void scanIR()
{
//  char        mykey[20] ;                                   // For numerated key
//  String      val ;                                         // Contents of preference entry
//  const char* reply ;                                       // Result of analyzeCmd
//  uint16_t    code ;                                        // The code of the Remote key
//  uint16_t    repeat_count ;                                // Will be increased by every repeated
                                                            // call to scanIR() until the key is released.
                                                            // Zero on first detection

  if ( ir.exitcommand != 0 )                                // Key release detected?
  {
    execute_ir_command ( "x" ) ;
    ir.exitcommand = 0;
    ir.exittimeout = 0;
  }
  if ( ir.protocol != IR_NONE )                             // Any input?
  {
//    char *protocol_names[] = {"", "NEC", "RC5"} ;           // Known IR protocol names
//    code = ir.command ;                                     // Key code is stored in lower word of ir_value
//    repeat_count = ir_value >> 16 ;                         // Repeat counter in upper word of if_value
    if ( 0 < ir.repeat_counter )                            // Is it a "longpress"?
    {                                                       // In case of a "longpress", we will first search
                                                            // for ir_XXXXrY, where XXXX is the HEX code of the
                                                            // key as usual and Y is the repeat counter in decimal,
                                                            // just as is, no leading '0'. Example: ir_40BFr20 means
                                                            // this is the 20th repeat of key with code $0BF. That
                                                            // translates (roughly) to a press time of (at least) 20 * 100ms
                                                            // If such a preference is not found, we will search for 
                                                            // ir_XXXXr, which fires for any repeat count.
                                                            // That means, ir_XXXXrY takes precedence, if defined for a
                                                            // specific repeat count, ir_XXXXr will NOT fire.
                                                            // (BTW: ir_XXXXr0 will never fire. ir_XXXX cannot be masked)
                                                            // If the 'R' is in uppercase, the event will fire and also 
                                                            // reset the repeat count. This is all handled in function                                                            
      execute_ir_command ( "r" );                           // execute_ir_command ( "r" );
    }
    else                                                    // On first press, do just search for ir_XXXX
    {
      execute_ir_command ( "" ) ;
    }
    ir.protocol = IR_NONE ;                                 // Indicate IR code has been handled to ISR
    ir.exittimeout = millis() ;                             // Set timeout for generating "key released event"  
  }
  else                                                      // No new key press/longpress detected
  {
    if (( ir.command != 0 ) && (ir.exittimeout != 0 ) )     // was there a valid keypress before
      if ( millis() - ir.exittimeout > 500 )                // That is too long ago (so no further repeat can be expected)
      {
        ir.exitcommand = ir.command ;                       // Set exitcode
        ir.command = 0 ;                                    // Clear code info  
        ir.repeat_counter = 0 ;                             // Clear repeat counter (just to be sure)
        ir.exittimeout = 0 ;                                // Clear exitcode timeout (jsut to be sure)
        execute_ir_command ( "x" );                         // Search NVS for "key release" ir_XXXXx
        ir.exitcommand = 0 ;                                // Clear exitcode after it has been used.
      }
  }
}


//**************************************************************************************************
//                                           M K _ L S A N                                         *
//**************************************************************************************************
// Make al list of acceptable networks in preferences.                                             *
// Will be called only once by setup().                                                            *
// The result will be stored in wifilist.                                                          *
// Not that the last found SSID and password are kept in common data.  If only one SSID is         *
// defined, the connect is made without using wifiMulti.  In this case a connection will           *
// be made even if de SSID is hidden.                                                              *
//**************************************************************************************************
void  mk_lsan()
{
  uint8_t     i ;                                        // Loop control
  char        key[10] ;                                  // For example: "wifi_03"
  String      buf ;                                      // "SSID/password"
  String      lssid, lpw ;                               // Last read SSID and password from nvs
  int         inx ;                                      // Place of "/"
  WifiInfo_t  winfo ;                                    // Element to store in list

  dbgprint ( "Create list with acceptable WiFi networks" ) ;
  for ( i = 0 ; i < 100 ; i++ )                          // Examine wifi_00 .. wifi_99
  {
    sprintf ( key, "wifi_%02d", i ) ;                    // Form key in preferences
    if ( nvssearch ( key  ) )                            // Does it exists?
    {
      buf = nvsgetstr ( key ) ;                          // Get the contents
      inx = buf.indexOf ( "/" ) ;                        // Find separator between ssid and password
      if ( inx > 0 )                                     // Separator found?
      {
        lpw = buf.substring ( inx + 1 ) ;                // Isolate password
        lssid = buf.substring ( 0, inx ) ;               // Holds SSID now
        dbgprint ( "Added %s to list of networks",
                   lssid.c_str() ) ;
        winfo.inx = i ;                                  // Create new element for wifilist ;
        winfo.ssid = strdup ( lssid.c_str() ) ;          // Set ssid of element
        winfo.passphrase = strdup ( lpw.c_str() ) ;
        wifilist.push_back ( winfo ) ;                   // Add to list
        wifiMulti.addAP ( winfo.ssid,                    // Add to wifi acceptable network list
                          winfo.passphrase ) ;
      }
    }
  }
  dbgprint ( "End adding networks" ) ; ////
}


//**************************************************************************************************
//                                     G E T R A D I O S T A T U S                                 *
//**************************************************************************************************
// Return preset-, tone- and volume status.                                                        *
// Included are the presets, the current station, the volume and the tone settings.                *
//**************************************************************************************************
String getradiostatus()
{
  char                pnr[3] ;                           // Preset as 2 character, i.e. "03"

  sprintf ( pnr, "%02d", ini_block.newpreset ) ;         // Current preset
  return String ( "preset=" ) +                          // Add preset setting
         String ( pnr ) +
         String ( "\nvolume=" ) +                        // Add volume setting
         String ( String ( ini_block.reqvol ) ) +
         String ( "\ntoneha=" ) +                        // Add tone setting HA
         String ( ini_block.rtone[0] ) +
         String ( "\ntonehf=" ) +                        // Add tone setting HF
         String ( ini_block.rtone[1] ) +
         String ( "\ntonela=" ) +                        // Add tone setting LA
         String ( ini_block.rtone[2] ) +
         String ( "\ntonelf=" ) +                        // Add tone setting LF
         String ( ini_block.rtone[3] ) ;
}


//**************************************************************************************************
//                                     G E T S E T T I N G S                                       *
//**************************************************************************************************
// Send some settings to the webserver.                                                            *
// Included are the presets, the current station, the volume and the tone settings.                *
//**************************************************************************************************
void getsettings()
{
  String              val ;                              // Result to send
  String              statstr ;                          // Station string
  int                 inx ;                              // Position of search char in line
  int16_t             i ;                                // Loop control, preset number

  for ( i = 0 ; i < MAXPRESETS ; i++ )                   // Max number of presets
  {
    statstr = readhostfrompref ( i ) ;                   // Get the preset from NVS
    if ( statstr != "" )                                 // Preset available?
    {
      // Show just comment if available.  Otherwise the preset itself.
      inx = statstr.indexOf ( "#" ) ;                    // Get position of "#"
      if ( inx > 0 )                                     // Hash sign present?
      {
        statstr.remove ( 0, inx + 1 ) ;                  // Yes, remove non-comment part
      }
      chomp ( statstr ) ;                                // Remove garbage from description
      dbgprint ( "statstr is %s", statstr.c_str() ) ;
      val += String ( "preset_" ) +
             String ( i ) +
             String ( "=" ) +
             statstr +
             String ( "\n" ) ;                           // Add delimeter
      if ( val.length() > 1000 )                         // Time to flush?
      {
        cmdclient.print ( val ) ;                        // Yes, send
        val = "" ;                                       // Start new string
      }
    }
  }
  val += getradiostatus() +                              // Add radio setting
         String ( "\n\n" ) ;                             // End of reply
  cmdclient.print ( val ) ;                              // And send
}


//**************************************************************************************************
//                                           T F T L O G                                           *
//**************************************************************************************************
// Log to TFT if enabled.                                                                          *
//**************************************************************************************************
void tftlog ( const char *str )
{
  if ( tft )                                           // TFT configured?
  {
    dsp_println ( str ) ;                              // Yes, show error on TFT
    dsp_update() ;                                     // To physical screen
  }
}


//**************************************************************************************************
//                                   F I N D N S I D                                               *
//**************************************************************************************************
// Find the namespace ID for the namespace passed as parameter.                                    *
//**************************************************************************************************
uint8_t FindNsID ( const char* ns )
{
  esp_err_t                 result = ESP_OK ;                 // Result of reading partition
  uint32_t                  offset = 0 ;                      // Offset in nvs partition
  uint8_t                   i ;                               // Index in Entry 0..125
  uint8_t                   bm ;                              // Bitmap for an entry
  uint8_t                   res = 0xFF ;                      // Function result

  while ( offset < nvs->size )
  {
    result = esp_partition_read ( nvs, offset,                // Read 1 page in nvs partition
                                  &nvsbuf,
                                  sizeof(nvsbuf) ) ;
    if ( result != ESP_OK )
    {
      dbgprint ( "Error reading NVS!" ) ;
      break ;
    }
    i = 0 ;
    while ( i < 126 )
    {

      bm = ( nvsbuf.Bitmap[i / 4] >> ( ( i % 4 ) * 2 ) ) ;    // Get bitmap for this entry,
      bm &= 0x03 ;                                            // 2 bits for one entry
      if ( ( bm == 2 ) &&
           ( nvsbuf.Entry[i].Ns == 0 ) &&
           ( strcmp ( ns, nvsbuf.Entry[i].Key ) == 0 ) )
      {
        res = nvsbuf.Entry[i].Data & 0xFF ;                   // Return the ID
        offset = nvs->size ;                                  // Stop outer loop as well
        break ;
      }
      else
      {
        if ( bm == 2 )
        {
          i += nvsbuf.Entry[i].Span ;                         // Next entry
        }
        else
        {
          i++ ;
        }
      }
    }
    offset += sizeof(nvs_page) ;                              // Prepare to read next page in nvs
  }
  return res ;
}


//**************************************************************************************************
//                            B U B B L E S O R T K E Y S                                          *
//**************************************************************************************************
// Bubblesort the nvskeys.                                                                         *
//**************************************************************************************************
void bubbleSortKeys ( uint16_t n )
{
  uint16_t i, j ;                                             // Indexes in nvskeys
  char     tmpstr[16] ;                                       // Temp. storage for a key

  for ( i = 0 ; i < n - 1 ; i++ )                             // Examine all keys
  {
    for ( j = 0 ; j < n - i - 1 ; j++ )                       // Compare to following keys
    {
      if ( strcmp ( nvskeys[j], nvskeys[j + 1] ) > 0 )        // Next key out of order?
      {
        strcpy ( tmpstr, nvskeys[j] ) ;                       // Save current key a while
        strcpy ( nvskeys[j], nvskeys[j + 1] ) ;               // Replace current with next key
        strcpy ( nvskeys[j + 1], tmpstr ) ;                   // Replace next with saved current
      }
    }
  }
}


//**************************************************************************************************
//                                      F I L L K E Y L I S T                                      *
//**************************************************************************************************
// File the list of all relevant keys in NVS.                                                      *
// The keys will be sorted.                                                                        *
//**************************************************************************************************
void fillkeylist()
{
  esp_err_t    result = ESP_OK ;                                // Result of reading partition
  uint32_t     offset = 0 ;                                     // Offset in nvs partition
  uint16_t     i ;                                              // Index in Entry 0..125.
  uint8_t      bm ;                                             // Bitmap for an entry
  uint16_t     nvsinx = 0 ;                                     // Index in nvskey table

  keynames.clear() ;                                            // Clear the list
  while ( offset < nvs->size )
  {
    result = esp_partition_read ( nvs, offset,                  // Read 1 page in nvs partition
                                  &nvsbuf,
                                  sizeof(nvsbuf) ) ;
    if ( result != ESP_OK )
    {
      dbgprint ( "Error reading NVS!" ) ;
      break ;
    }
    i = 0 ;
    while ( i < 126 )
    {
      bm = ( nvsbuf.Bitmap[i / 4] >> ( ( i % 4 ) * 2 ) ) ;      // Get bitmap for this entry,
      bm &= 0x03 ;                                              // 2 bits for one entry
      if ( bm == 2 )                                            // Entry is active?
      {
        if ( nvsbuf.Entry[i].Ns == namespace_ID )               // Namespace right?
        {
          strcpy ( nvskeys[nvsinx], nvsbuf.Entry[i].Key ) ;     // Yes, save in table
          if ( ++nvsinx == MAXKEYS )
          {
            nvsinx-- ;                                          // Prevent excessive index
          }
        }
        i += nvsbuf.Entry[i].Span ;                             // Next entry
      }
      else
      {
        i++ ;
      }
    }
    offset += sizeof(nvs_page) ;                                // Prepare to read next page in nvs
  }
  nvskeys[nvsinx][0] = '\0' ;                                   // Empty key at the end
  dbgprint ( "Read %d keys from NVS", nvsinx ) ;
  bubbleSortKeys ( nvsinx ) ;                                   // Sort the keys
}


//**************************************************************************************************
//                                           S E T U P                                             *
//**************************************************************************************************
// Setup for the program.                                                                          *
//**************************************************************************************************
void setup()
{
  int                       i ;                          // Loop control
  int                       pinnr ;                      // Input pinnumber
  const char*               p ;
  byte                      mac[6] ;                     // WiFi mac address
  char                      tmpstr[20] ;                 // For version and Mac address
  const char*               partname = "nvs" ;           // Partition with NVS info
  esp_partition_iterator_t  pi ;                         // Iterator for find
  const char*               dtyp = "Display type is %s" ;
  const char*               wvn = "Include file %s_html has the wrong version number! "
                                  "Replace header file." ;

  Serial.begin ( 115200 ) ;                              // For debug
  Serial.println() ;
  // Version tests for some vital include files
  if ( about_html_version   < 170626 ) dbgprint ( wvn, "about" ) ;
  if ( config_html_version  < 180806 ) dbgprint ( wvn, "config" ) ;
  if ( index_html_version   < 180102 ) dbgprint ( wvn, "index" ) ;
  if ( mp3play_html_version < 180918 ) dbgprint ( wvn, "mp3play" ) ;
  if ( defaultprefs_version < 180816 ) dbgprint ( wvn, "defaultprefs" ) ;
  // Print some memory and sketch info
  dbgprint ( "Starting ESP32-radio running on CPU %d at %d MHz.  Version %s.  Free memory %d",
             xPortGetCoreID(),
             ESP.getCpuFreqMHz(),
             VERSION,
             ESP.getFreeHeap() ) ;                       // Normally about 170 kB
#if defined ( BLUETFT )                                // Report display option
  dbgprint ( dtyp, "BLUETFT" ) ;
#endif
#if defined ( ILI9341 )                                // Report display option
  dbgprint ( dtyp, "ILI9341" ) ;
#endif
#if defined ( OLED )
  dbgprint ( dtyp, "OLED" ) ;
#endif
#if defined ( DUMMYTFT )
  dbgprint ( dtyp, "DUMMYTFT" ) ;
#endif
#if defined ( LCD1602I2C )
  dbgprint ( dtyp, "LCD1602" ) ;
#endif
#if defined ( LCD2004I2C )
  dbgprint ( dtyp, "LCD2004" ) ;
#endif
#if defined ( NEXTION )
  dbgprint ( dtyp, "NEXTION" ) ;
#endif
  maintask = xTaskGetCurrentTaskHandle() ;               // My taskhandle
  SPIsem = xSemaphoreCreateMutex(); ;                    // Semaphore for SPI bus
  pi = esp_partition_find ( ESP_PARTITION_TYPE_DATA,     // Get partition iterator for
                            ESP_PARTITION_SUBTYPE_ANY,   // the NVS partition
                            partname ) ;
  if ( pi )
  {
    nvs = esp_partition_get ( pi ) ;                     // Get partition struct
    esp_partition_iterator_release ( pi ) ;              // Release the iterator
    dbgprint ( "Partition %s found, %d bytes",
               partname,
               nvs->size ) ;
  }
  else
  {
    dbgprint ( "Partition %s not found!", partname ) ;   // Very unlikely...
    while ( true ) ;                                     // Impossible to continue
  }
  namespace_ID = FindNsID ( NAME ) ;                     // Find ID of our namespace in NVS
  fillkeylist() ;                                        // Fill keynames with all keys
  memset ( &ini_block, 0, sizeof(ini_block) ) ;          // Init ini_block
  ini_block.mqttport = 1883 ;                            // Default port for MQTT
  ini_block.mqttprefix = "" ;                            // No prefix for MQTT topics seen yet
  ini_block.clk_server = "pool.ntp.org" ;                // Default server for NTP
  ini_block.clk_offset = 1 ;                             // Default Amsterdam time zone
  ini_block.clk_dst = 1 ;                                // DST is +1 hour
  ini_block.bat0 = 0 ;                                   // Battery ADC levels not yet defined
  ini_block.bat100 = 0 ;
#if defined(RETRORADIO)
  ini_block.vol_min = 0;
  ini_block.vol_max = 100;
#endif

  readIOprefs() ;                                        // Read pins used for SPI, TFT, VS1053, IR,
                                                         // Rotary encoder
  for ( i = 0 ; (pinnr = progpin[i].gpio) >= 0 ; i++ )   // Check programmable input pins
  {
    pinMode ( pinnr, INPUT_PULLUP ) ;                    // Input for control button
    delay ( 10 ) ;
    // Check if pull-up active
    if ( ( progpin[i].cur = digitalRead ( pinnr ) ) == HIGH )
    {
      p = "HIGH" ;
    }
    else
    {
      p = "LOW, probably no PULL-UP" ;                   // No Pull-up
    }
    dbgprint ( "GPIO%d is %s", pinnr, p ) ;
  }
  readprogbuttons() ;                                    // Program the free input pins
  SPI.begin ( ini_block.spi_sck_pin,                     // Init VSPI bus with default or modified pins
              ini_block.spi_miso_pin,
              ini_block.spi_mosi_pin ) ;
  vs1053player = new VS1053 ( ini_block.vs_cs_pin,       // Make instance of player
                              ini_block.vs_dcs_pin,
                              ini_block.vs_dreq_pin,
                              ini_block.vs_shutdown_pin,
                              ini_block.vs_shutdownx_pin ) ;
  if ( ini_block.ir_pin >= 0 )
  {
    dbgprint ( "Enable pin %d for IR (NEC and RC5)",
               ini_block.ir_pin ) ;
    pinMode ( ini_block.ir_pin, INPUT ) ;                // Pin for IR receiver VS1838B
    attachInterrupt ( ini_block.ir_pin,                  // Interrupts will be handle by isr_IR
                      isr_IR, CHANGE ) ;
  }
  if ( ini_block.ir_pin_nec >= 0 )
  {
    ini_block.ir_pin = ini_block.ir_pin_nec ;            // ISR reads the pin. So decision which pin to read does not arise in ISR.
    dbgprint ( "Enable pin %d for IR (NEC)",
               ini_block.ir_pin ) ;
    pinMode ( ini_block.ir_pin, INPUT ) ;                // Pin for IR receiver VS1838B
    attachInterrupt ( ini_block.ir_pin,                  // Interrupts will be handle by isr_IR
                      isr_IRNEC, CHANGE ) ;
  }
  if ( ini_block.ir_pin_rc5 >= 0 )
  {
    ini_block.ir_pin = ini_block.ir_pin_rc5 ;            // ISR reads the pin. So decision which pin to read does not arise in ISR.
    dbgprint ( "Enable pin %d for IR (RC5)",
               ini_block.ir_pin ) ;
    pinMode ( ini_block.ir_pin, INPUT ) ;                // Pin for IR receiver VS1838B
    attachInterrupt ( ini_block.ir_pin,                  // Interrupts will be handle by isr_IR
                      isr_IRRC5, CHANGE ) ;
  }
  if ( ( ini_block.tft_cs_pin >= 0  ) ||                 // Display configured?
       ( ini_block.tft_scl_pin >= 0 ) )
  {
    dbgprint ( "Start display" ) ;
    if ( dsp_begin() )                                   // Init display
    {
      dsp_setRotation() ;                                // Use landscape format
      dsp_erase() ;                                      // Clear screen
      dsp_setTextSize ( 1 ) ;                            // Small character font
      dsp_setTextColor ( WHITE ) ;                       // Info in white
      dsp_setCursor ( 0, 0 ) ;                           // Top of screen
      dsp_print ( "Starting..." "\n" "Version:" ) ;
      strncpy ( tmpstr, VERSION, 16 ) ;                  // Limit version length
      dsp_println ( tmpstr ) ;
      dsp_println ( "By Ed Smallenburg" ) ;
      dsp_update() ;                                     // Show on physical screen
    }
  }
  if ( ini_block.tft_bl_pin >= 0 )                       // Backlight for TFT control?
  {
    pinMode ( ini_block.tft_bl_pin, OUTPUT ) ;           // Yes, enable output
  }
  if ( ini_block.tft_blx_pin >= 0 )                      // Backlight for TFT (inversed logic) control?
  {
    pinMode ( ini_block.tft_blx_pin, OUTPUT ) ;          // Yes, enable output
  }
  blset ( true ) ;                                       // Enable backlight (if configured)
  setup_SDCARD() ;                                       // Set-up SD card (if configured)

  mk_lsan() ;                                            // Make a list of acceptable networks
                                                         // in preferences.
  readprefs ( false ) ;                                  // Read preferences
  vs1053player->begin() ;                                // Initialize VS1053 player
  WiFi.mode ( WIFI_STA ) ;                               // This ESP is a station
  WiFi.persistent ( false ) ;                            // Do not save SSID and password
  WiFi.disconnect() ;                                    // After restart router could still
#if (ETHERNET==1)
  adc_power_on();                                        // Workaround to allow GPIO36/39 as IR-Input
  NetworkFound = connecteth();                           // Try ethernet
  if (NetworkFound) {                                    // Ethernet success??
    WiFi.mode(WIFI_OFF);
  }
#else
  delay ( 500 ) ;                                        // keep old connection
#endif
  if (!NetworkFound) {  
    //WiFi.mode ( WIFI_STA ) ;                               // This ESP is a station
    //delay ( 500 ) ;                                        // ??
    //WiFi.persistent ( false ) ;                            // Do not save SSID and password
    listNetworks() ;                                       // Find WiFi networks
    tcpip_adapter_set_hostname ( TCPIP_ADAPTER_IF_STA,
                               NAME ) ;
    p = dbgprint ( "Connect to WiFi" ) ;                   // Show progress
    tftlog ( p ) ;                                         // On TFT too
    NetworkFound = connectwifi() ;                         // Connect to WiFi network
  }

  dbgprint ( "Start server for commands" ) ;
  cmdserver.begin() ;                                    // Start http server
  if ( NetworkFound )                                    // OTA and MQTT only if Wifi network found
  {
    dbgprint ( "Network found. Starting mqtt and OTA" ) ;
    mqtt_on = ( ini_block.mqttbroker.length() > 0 ) &&   // Use MQTT if broker specified
              ( ini_block.mqttbroker != "none" ) ;
    ArduinoOTA.setHostname ( NAME ) ;                    // Set the hostname
    ArduinoOTA.onStart ( otastart ) ;
    ArduinoOTA.begin() ;                                 // Allow update over the air
 //   if (EthernetFound)
 //     SerialBT.begin(NAME); 
    if ( mqtt_on )                                       // Broker specified?
    {
      if ( ( ini_block.mqttprefix.length() == 0 ) ||     // No prefix?
           ( ini_block.mqttprefix == "none" ) )
      {
        WiFi.macAddress ( mac ) ;                        // Get mac-adress
        sprintf ( tmpstr, "P%02X%02X%02X%02X",           // Generate string from last part
                  mac[3], mac[2],
                  mac[1], mac[0] ) ;
        ini_block.mqttprefix = String ( tmpstr ) ;       // Save for further use
      }
      dbgprint ( "MQTT uses prefix %s", ini_block.mqttprefix.c_str() ) ;
      dbgprint ( "Init MQTT" ) ;
      mqttclient.setServer(ini_block.mqttbroker.c_str(), // Specify the broker
                           ini_block.mqttport ) ;        // And the port
      mqttclient.setCallback ( onMqttMessage ) ;         // Set callback on receive
    }
    if ( MDNS.begin ( NAME ) )                           // Start MDNS transponder
    {
      dbgprint ( "MDNS responder started" ) ;
    }
    else
    {
      dbgprint ( "Error setting up MDNS responder!" ) ;
    }
  }
  else
  {
    currentpreset = ini_block.newpreset ;                // No network: do not start radio
  }
  timer = timerBegin ( 0, 80, true ) ;                   // User 1st timer with prescaler 80
  timerAttachInterrupt ( timer, &timer100, true ) ;      // Call timer100() on timer alarm
  timerAlarmWrite ( timer, 100000, true ) ;              // Alarm every 100 msec
  timerAlarmEnable ( timer ) ;                           // Enable the timer
  delay ( 1000 ) ;                                       // Show IP for a while
  configTime ( ini_block.clk_offset * 3600,
               ini_block.clk_dst * 3600,
               ini_block.clk_server.c_str() ) ;          // GMT offset, daylight offset in seconds
  timeinfo.tm_year = 0 ;                                 // Set TOD to illegal
  // Init settings for rotary switch (if existing).
  if ( ( ini_block.enc_clk_pin + ini_block.enc_dt_pin + ini_block.enc_sw_pin ) > 2 )
  {
    attachInterrupt ( ini_block.enc_clk_pin, isr_enc_turn,   CHANGE ) ;
    attachInterrupt ( ini_block.enc_dt_pin,  isr_enc_turn,   CHANGE ) ;
    attachInterrupt ( ini_block.enc_sw_pin,  isr_enc_switch, CHANGE ) ;
    dbgprint ( "Rotary encoder is enabled" ) ;
  }
  else
  {
    dbgprint ( "Rotary encoder is disabled (%d/%d/%d)",
               ini_block.enc_clk_pin,
               ini_block.enc_dt_pin,
               ini_block.enc_sw_pin) ;
  }
  if ( NetworkFound )
  {
    gettime() ;                                           // Sync time
  }
  if ( tft )
  {
    dsp_fillRect ( 0, 8,                                  // Clear most of the screen
                   dsp_getwidth(),
                   dsp_getheight() - 8, BLACK ) ;
  }
  outchunk.datatyp = QDATA ;                              // This chunk dedicated to QDATA
  adc1_config_width ( ADC_WIDTH_12Bit ) ;
  adc1_config_channel_atten ( ADC1_CHANNEL_0, ADC_ATTEN_0db ) ;
  dataqueue = xQueueCreate ( QSIZ,                        // Create queue for communication
                             sizeof ( qdata_struct ) ) ;
  xTaskCreatePinnedToCore (
    playtask,                                             // Task to play data in dataqueue.
    "Playtask",                                           // name of task.
    1600,                                                 // Stack size of task
    NULL,                                                 // parameter of the task
    2,                                                    // priority of the task
    &xplaytask,                                           // Task handle to keep track of created task
    0 ) ;                                                 // Run on CPU 0
  xTaskCreate (
    spftask,                                              // Task to handle special functions.
    "Spftask",                                            // name of task.
    2048,                                                 // Stack size of task
    NULL,                                                 // parameter of the task
    1,                                                    // priority of the task
    &xspftask ) ;                                         // Task handle to keep track of created task
}


//**************************************************************************************************
//                                        R I N B Y T                                              *
//**************************************************************************************************
// Read next byte from http inputbuffer.  Buffered for speed reasons.                              *
//**************************************************************************************************
uint8_t rinbyt ( bool forcestart )
{
  static uint8_t  buf[1024] ;                           // Inputbuffer
  static uint16_t i ;                                   // Pointer in inputbuffer
  static uint16_t len ;                                 // Number of bytes in buf
  uint16_t        tlen ;                                // Number of available bytes
  uint16_t        trycount = 0 ;                        // Limit max. time to read

  if ( forcestart || ( i == len ) )                     // Time to read new buffer
  {
    while ( cmdclient.connected() )                     // Loop while the client's connected
    {
      tlen = cmdclient.available() ;                    // Number of bytes to read from the client
      len = tlen ;                                      // Try to read whole input
      if ( len == 0 )                                   // Any input available?
      {
        if ( ++trycount > 3 )                           // Not for a long time?
        {
          dbgprint ( "HTTP input shorter than expected" ) ;
          return '\n' ;                                 // Error! No input
        }
        delay ( 10 ) ;                                  // Give communication some time
        continue ;                                      // Next loop of no input yet
      }
      if ( len > sizeof(buf) )                          // Limit number of bytes
      {
        len = sizeof(buf) ;
      }
      len = cmdclient.read ( buf, len ) ;               // Read a number of bytes from the stream
      i = 0 ;                                           // Pointer to begin of buffer
      break ;
    }
  }
  return buf[i++] ;
}


//**************************************************************************************************
//                                        W R I T E P R E F S                                      *
//**************************************************************************************************
// Update the preferences.  Called from the web interface.                                         *
//**************************************************************************************************
void writeprefs()
{
  int        inx ;                                            // Position in inputstr
  uint8_t    winx ;                                           // Index in wifilist
  char       c ;                                              // Input character
  String     inputstr = "" ;                                  // Input regel
  String     key, contents ;                                  // Pair for Preferences entry
  String     dstr ;                                           // Contents for debug

  timerAlarmDisable ( timer ) ;                               // Disable the timer
  nvsclear() ;                                                // Remove all preferences
  while ( true )
  {
    c = rinbyt ( false ) ;                                    // Get next inputcharacter
    if ( c == '\n' )                                          // Newline?
    {
      if ( inputstr.length() == 0 )
      {
        dbgprint ( "End of writing preferences" ) ;
        break ;                                               // End of contents
      }
      if ( !inputstr.startsWith ( "#" ) )                     // Skip pure comment lines
      {
        inx = inputstr.indexOf ( "=" ) ;
        if ( inx >= 0 )                                       // Line with "="?
        {
          key = inputstr.substring ( 0, inx ) ;               // Yes, isolate the key
          key.trim() ;
          contents = inputstr.substring ( inx + 1 ) ;         // and contents
          contents.trim() ;
          dstr = contents ;                                   // Copy for debug
          if ( ( key.indexOf ( "wifi_" ) == 0 ) )             // Sensitive info?
          {
            winx = key.substring(5).toInt() ;                 // Get index in wifilist
            if ( ( winx < wifilist.size() ) &&                // Existing wifi spec in wifilist?
                 ( contents.indexOf ( wifilist[winx].ssid ) == 0 ) &&
                 ( contents.indexOf ( "/****" ) > 0 ) )       // Hidden password?
            {
              contents = String ( wifilist[winx].ssid ) +     // Retrieve ssid and password
                         String ( "/" ) +
                         String ( wifilist[winx].passphrase ) ;
              dstr = String ( wifilist[winx].ssid ) +
                     String ( "/*******" ) ;                  // Hide in debug line
            }
          }
          if ( ( key.indexOf ( "mqttpasswd" ) == 0 ) )        // Sensitive info?
          {
            if ( contents.indexOf ( "****" ) == 0 )           // Hidden password?
            {
              contents = ini_block.mqttpasswd ;               // Retrieve mqtt password
            }
            dstr = String ( "*******" ) ;                     // Hide in debug line
          }
          dbgprint ( "writeprefs setstr %s = %s",
                     key.c_str(), dstr.c_str() ) ;
          nvssetstr ( key.c_str(), contents ) ;               // Save new pair
        }
      }
      inputstr = "" ;
    }
    else
    {
      if ( c != '\r' )                                        // Not newline.  Is is a CR?
      {
        inputstr += String ( c ) ;                            // No, normal char, add to string
      }
    }
  }
  timerAlarmEnable ( timer ) ;                                // Enable the timer
  fillkeylist() ;                                             // Update list with keys
}


//**************************************************************************************************
//                                        H A N D L E H T T P R E P L Y                            *
//**************************************************************************************************
// Handle the output after an http request.                                                        *
//**************************************************************************************************
void handlehttpreply()
{
  const char*   p ;                                         // Pointer to reply if command
  String        sndstr = "" ;                               // String to send
  int           n ;                                         // Number of files on SD card

  if ( http_response_flag )
  {
    http_response_flag = false ;
    if ( cmdclient.connected() )
    {
      if ( http_rqfile.length() == 0 &&                     // An empty "GET"?
           http_getcmd.length() == 0 )
      {
        if ( NetworkFound )                                 // Yes, check network
        {
          handleFSf ( String( "index.html") ) ;             // Okay, send the startpage
        }
        else
        {
          handleFSf ( String( "config.html") ) ;            // Or the configuration page if in AP mode
        }
      }
      else
      {
        if ( http_getcmd.length() )                         // Command to analyze?
        {
          dbgprint ( "Send reply for %s", http_getcmd.c_str() ) ;
          sndstr = httpheader ( String ( "text/html" ) ) ;  // Set header
          if ( http_getcmd.startsWith ( "getprefs" ) )      // Is it a "Get preferences"?
          {
            if ( datamode != STOPPED )                      // Still playing?
            {
              setdatamode (  STOPREQD ) ;                   // Stop playing
            }
            sndstr += readprefs ( true ) ;                  // Read and send
          }
          else if ( http_getcmd.startsWith ( "getdefs" ) )  // Is it a "Get default preferences"?
          {
            sndstr += String ( defprefs_txt + 1 ) ;         // Yes, read initial values
          }
          else if ( http_getcmd.startsWith ("saveprefs") )  // Is is a "Save preferences"
          {
            writeprefs() ;                                  // Yes, handle it
            sndstr += String ( "Config saved" ) ;           // Give reply
          }
          else if ( http_getcmd.startsWith ( "mp3list" ) )  // Is is a "Get SD MP3 tracklist"?
          {
            if ( datamode != STOPPED )                      // Still playing?
            {
              setdatamode ( STOPREQD ) ;                    // Stop playing
            }
            cmdclient.print ( sndstr ) ;                    // Yes, send header
            n = listfstracks ( "/", 0, true ) ;             // Handle it
            dbgprint ( "%d tracks on local drive", n ) ;
            return ;                                        // Do not send empty line
          }
          else if ( http_getcmd.startsWith ( "settings" ) ) // Is is a "Get settings" (like presets and tone)?
          {
            cmdclient.print ( sndstr ) ;                    // Yes, send header
            getsettings() ;                                 // Handle settings request
            return ;                                        // Do not send empty line
          }
          else
          {
            p = analyzeCmd ( http_getcmd.c_str() ) ;        // Yes, do so
            sndstr += String ( p ) ;                        // Content of HTTP response follows the header
          }
          sndstr += String ( "\n" ) ;                       // The HTTP response ends with a blank line
          cmdclient.print ( sndstr ) ;
        }
        else if ( http_rqfile.length() )                    // File requested?
        {
          dbgprint ( "Start file reply for %s",
                     http_rqfile.c_str() ) ;
          handleFSf ( http_rqfile ) ;                       // Yes, send it
        }
        else
        {
          httpheader ( String ( "text/html" ) ) ;           // Send header
          // the content of the HTTP response follows the header:
          cmdclient.println ( "Dummy response\n" ) ;        // Text ending with double newline
          dbgprint ( "Dummy response sent" ) ;
        }
      }
    }
  }
}


//**************************************************************************************************
//                                        H A N D L E H T T P                                      *
//**************************************************************************************************
// Handle the input of an http request.                                                            *
//**************************************************************************************************
void handlehttp()
{
  bool        first = true ;                                 // First call to rinbyt()
  char        c ;                                            // Next character from http input
  int         inx0, inx ;                                    // Pos. of search string in currenLine
  String      currentLine = "" ;                             // Build up to complete line
  bool        reqseen = false ;                              // No GET seen yet

  if ( !cmdclient.connected() )                              // Action if client is connected
  {
    return ;                                                 // No client active
  }
  dbgprint ( "handlehttp started" ) ;
  while ( true )                                             // Loop till command/file seen
  {
    c = rinbyt ( first ) ;                                   // Get a byte
    first = false ;                                          // No more first call
    if ( c == '\n' )
    {
      // If the current line is blank, you got two newline characters in a row.
      // that's the end of the client HTTP request, so send a response:
      if ( currentLine.length() == 0 )
      {
        http_response_flag = reqseen ;                       // Response required or not
        break ;
      }
      else
      {
        // Newline seen, remember if it is like "GET /xxx?y=2&b=9 HTTP/1.1"
        if ( currentLine.startsWith ( "GET /" ) )            // GET request?
        {
          inx0 = 5 ;                                         // Start search at pos 5
        }
        else if ( currentLine.startsWith ( "POST /" ) )      // POST request?
        {
          inx0 = 6 ;
        }
        else
        {
          inx0 = 0 ;                                         // Not GET nor POST
        }
        if ( inx0 )                                          // GET or POST request?
        {
          reqseen = true ;                                   // Request seen
          inx = currentLine.indexOf ( "&" ) ;                // Search for 2nd parameter
          if ( inx < 0 )
          {
            inx = currentLine.indexOf ( " HTTP" ) ;          // Search for end of GET command
          }
          // Isolate the command
          http_getcmd = currentLine.substring ( inx0, inx ) ;
          inx = http_getcmd.indexOf ( "?" ) ;                // Search for command
          if ( inx == 0 )                                    // Arguments only?
          {
            http_getcmd = http_getcmd.substring ( 1 ) ;      // Yes, get rid of question mark
            http_rqfile = "" ;                               // No file
          }
          else if ( inx > 0 )                                // Filename present?
          {
            http_rqfile = http_getcmd.substring ( 0, inx ) ; // Remember filename
            http_getcmd = http_getcmd.substring ( inx + 1 ) ; // Remove filename from GET command
          }
          else
          {
            http_rqfile = http_getcmd ;                      // No parameters, set filename
            http_getcmd = "" ;
          }
          if ( http_getcmd.length() )
          {
            dbgprint ( "Get command is: %s",                 // Show result
                       http_getcmd.c_str() ) ;
          }
          if ( http_rqfile.length() )
          {
            dbgprint ( "Filename is: %s",                    // Show requested file
                       http_rqfile.c_str() ) ;
          }
        }
        currentLine = "" ;
      }
    }
    else if ( c != '\r' )                                    // No LINFEED.  Is it a CR?
    {
      currentLine += c ;                                     // No, add normal char to currentLine
    }
  }
  //cmdclient.stop() ;
}


//**************************************************************************************************
//                                          X M L P A R S E                                        *
//**************************************************************************************************
// Parses line with XML data and put result in variable specified by parameter.                    *
//**************************************************************************************************
void xmlparse ( String &line, const char *selstr, String &res )
{
  String sel = "</" ;                                  // Will be like "</status-code"
  int    inx ;                                         // Position of "</..." in line

  sel += selstr ;                                      // Form searchstring
  if ( line.endsWith ( sel ) )                         // Is this the line we are looking for?
  {
    inx = line.indexOf ( sel ) ;                       // Get position of end tag
    res = line.substring ( 0, inx ) ;                  // Set result
  }
}


//**************************************************************************************************
//                                          X M L G E T H O S T                                    *
//**************************************************************************************************
// Parses streams from XML data.                                                                   *
// Example URL for XML Data Stream:                                                                *
// http://playerservices.streamtheworld.com/api/livestream?version=1.5&mount=IHR_TRANAAC&lang=en   *
//**************************************************************************************************
String xmlgethost  ( String mount )
{
  const char* xmlhost = "playerservices.streamtheworld.com" ;  // XML data source
  const char* xmlget =  "GET /api/livestream"                  // XML get parameters
                        "?version=1.5"                         // API Version of IHeartRadio
                        "&mount=%sAAC"                         // MountPoint with Station Callsign
                        "&lang=en" ;                           // Language

  String   stationServer = "" ;                     // Radio stream server
  String   stationPort = "" ;                       // Radio stream port
  String   stationMount = "" ;                      // Radio stream Callsign
  uint16_t timeout = 0 ;                            // To detect time-out
  String   sreply = "" ;                            // Reply from playerservices.streamtheworld.com
  String   statuscode = "200" ;                     // Assume good reply
  char     tmpstr[200] ;                            // Full GET command, later stream URL
  String   urlout ;                                 // Result URL

  stop_mp3client() ; // Stop any current wificlient connections.
  dbgprint ( "Connect to new iHeartRadio host: %s", mount.c_str() ) ;
  setdatamode ( INIT ) ;                            // Start default in metamode
  chunked = false ;                                   // Assume not chunked
  sprintf ( tmpstr, xmlget, mount.c_str() ) ;         // Create a GET commmand for the request
  dbgprint ( "%s", tmpstr ) ;
  if ( mp3client.connect ( xmlhost, 80 ) )            // Connect to XML stream
  {
    dbgprint ( "Connected to %s", xmlhost ) ;
    mp3client.print ( String ( tmpstr ) + " HTTP/1.1\r\n"
                      "Host: " + xmlhost + "\r\n"
                      "User-Agent: Mozilla/5.0\r\n"
                      "Connection: close\r\n\r\n" ) ;
    while ( mp3client.available() == 0 )
    {
      delay ( 200 ) ;                                 // Give server some time
      if ( ++timeout > 25 )                           // No answer in 5 seconds?
      {
        dbgprint ( "Client Timeout !" ) ;
      }
    }
    dbgprint ( "XML parser processing..." ) ;
    while ( mp3client.available() )
    {
      sreply = mp3client.readStringUntil ( '>' ) ;
      sreply.trim() ;
      // Search for relevant info in in reply and store in variable
      xmlparse ( sreply, "status-code", statuscode ) ;
      xmlparse ( sreply, "ip",          stationServer ) ;
      xmlparse ( sreply, "port",        stationPort ) ;
      xmlparse ( sreply, "mount",       stationMount ) ;
      if ( statuscode != "200" )                      // Good result sofar?
      {
        dbgprint ( "Bad xml status-code %s",         // No, show and stop interpreting
                   statuscode.c_str() ) ;
        tmpstr[0] = '\0' ;                           // Clear result
        break ;
      }
    }
    if ( ( stationServer != "" ) &&                   // Check if all station values are stored
         ( stationPort != "" ) &&
         ( stationMount != "" ) )
    {
      sprintf ( tmpstr, "%s:%s/%s_SC",                // Build URL for ESP-Radio to stream.
                stationServer.c_str(),
                stationPort.c_str(),
                stationMount.c_str() ) ;
      dbgprint ( "Found: %s", tmpstr ) ;
    }
  }
  else
  {
    dbgprint ( "Can't connect to XML host!" ) ;       // Connection failed
    tmpstr[0] = '\0' ;
  }
  mp3client.stop() ;
  return String ( tmpstr ) ;                          // Return final streaming URL.
}


//**************************************************************************************************
//                                      H A N D L E S A V E R E Q                                  *
//**************************************************************************************************
// Handle save volume/preset/tone.  This will save current settings every 10 minutes to            *
// the preferences.  On the next restart these values will be loaded.                              *
// Note that saving prefences will only take place if contents has changed.                        *
//**************************************************************************************************
void handleSaveReq()
{
  static uint32_t savetime = 0 ;                          // Limit save to once per 10 minutes

  if ( ( millis() - savetime ) < 600000 )                 // 600 sec is 10 minutes
  {
    return ;
  }
  savetime = millis() ;                                   // Set time of last save
  //nvssetstr ( "preset", String ( currentpreset )  ) ;     // Save current preset
  //nvssetstr ( "volume", String ( ini_block.reqvol ) );    // Save current volue
  nvssetstr ( "toneha", String ( ini_block.rtone[0] ) ) ; // Save current toneha
  nvssetstr ( "tonehf", String ( ini_block.rtone[1] ) ) ; // Save current tonehf
  nvssetstr ( "tonela", String ( ini_block.rtone[2] ) ) ; // Save current tonela
  nvssetstr ( "tonelf", String ( ini_block.rtone[3] ) ) ; // Save current tonelf
}


//**************************************************************************************************
//                                      H A N D L E I P P U B                                      *
//**************************************************************************************************
// Handle publish op IP to MQTT.  This will happen every 10 minutes.                               *
//**************************************************************************************************
void handleIpPub()
{
  static uint32_t pubtime = 300000 ;                       // Limit save to once per 10 minutes

  if ( ( millis() - pubtime ) < 600000 )                   // 600 sec is 10 minutes
  {
    return ;
  }
  pubtime = millis() ;                                     // Set time of last publish
  mqttpub.trigger ( MQTT_IP ) ;                            // Request re-publish IP
}


//**************************************************************************************************
//                                      H A N D L E V O L P U B                                    *
//**************************************************************************************************
// Handle publish of Volume to MQTT.  This will happen max every 10 seconds.                       *
//**************************************************************************************************
void handleVolPub()
{
  static uint32_t pubtime = 10000 ;                        // Limit save to once per 10 seconds
  static uint8_t  oldvol = -1 ;                            // For comparison

  if ( ( millis() - pubtime ) < 10000 )                    // 10 seconds
  {
    return ;
  }
  pubtime = millis() ;                                     // Set time of last publish
  if ( ini_block.reqvol != oldvol )                        // Volume change?
  {
    mqttpub.trigger ( MQTT_VOLUME ) ;                      // Request publish VOLUME
    oldvol = ini_block.reqvol ;                            // Remember publishe volume
  }
}


//**************************************************************************************************
//                                     G E T F S F I L E N A M E                                   *
//**************************************************************************************************
// Get filename for a node ID.  Search on SD or USB.                                               *
//**************************************************************************************************
String getFSfilename ( String &nodeID )
{
  if ( ( usb_sd == FS_USB ) && USB_okay )              // File system depends on this switch
  {
    return getUSBfilename ( nodeID ) ;                 // like "localhost/........"
  }
  else if ( ( usb_sd == FS_SD ) && SD_okay )
  {
    return getSDfilename ( nodeID ) ;                  // like "localhost/........"
  }
  return String ( "" ) ;                               // Not found
}


//**************************************************************************************************
//                                           C H K _ E N C                                         *
//**************************************************************************************************
// See if rotary encoder is activated and perform its functions.                                   *
//**************************************************************************************************
void chk_enc()
{
  static int8_t  enc_preset ;                                 // Selected preset
  static String  enc_nodeID ;                                 // Node of selected track
  static String  enc_filename ;                               // Filename of selected track
  String         tmp ;                                        // Temporary string
  int16_t        inx ;                                        // Position in string
  String         rt = "0" ;                                   // NodeID for random track

  if ( enc_menu_mode != VOLUME )                              // In default mode?
  {
    if ( enc_inactivity > 40 )                                // No, more than 4 seconds inactive
    {
      enc_inactivity = 0 ;
      enc_menu_mode = VOLUME ;                                // Return to VOLUME mode
      dbgprint ( "Encoder mode back to VOLUME" ) ;
      tftset ( 2, (char*)NULL ) ;                             // Restore original text at bottom
    }
  }
  if ( singleclick || doubleclick ||                          // Any activity?
       tripleclick || longclick ||
       ( rotationcount != 0 ) )
  {
    blset ( true ) ;                                          // Yes, activate display if needed
  }
  else
  {
    return ;                                                  // No, nothing to do
  }
  if ( tripleclick )                                          // First handle triple click
  {
    dbgprint ( "Triple click") ;
    tripleclick = false ;
    if ( get_FS_nodecount() )                                 // Tracks on FS?
    {
      enc_menu_mode = TRACK ;                                 // Swich to TRACK mode
      dbgprint ( "Encoder mode set to TRACK" ) ;
      tftset ( 3, "Turn to select track\n"                    // Show current option
               "Press to confirm" ) ;
      enc_nodeID = selectnextFSnode ( +1 ) ;                  // Start with next file on SD/USB
      if ( enc_nodeID == "" )                                 // Current track available?
      {
        inx = SD_nodelist.indexOf ( "\n" ) ;                  // No, find first
        enc_nodeID = SD_nodelist.substring ( 0, inx ) ;
      }
      // Stop playing as reading filenames saturates SD I/O.
      if ( datamode != STOPPED )
      {
        setdatamode ( STOPREQD ) ;                            // Request STOP
      }
    }
  }
  if ( doubleclick )                                          // Handle the doubleclick
  {
    dbgprint ( "Double click") ;
    doubleclick = false ;
    enc_menu_mode = PRESET ;                                  // Swich to PRESET mode
    dbgprint ( "Encoder mode set to PRESET" ) ;
    tftset ( 3, "Turn to select station\n"                    // Show current option
             "Press to confirm" ) ;
    enc_preset = ini_block.newpreset + 1 ;                    // Start with current preset + 1
  }
  if ( singleclick )
  {
    dbgprint ( "Single click") ;
    singleclick = false ;
    switch ( enc_menu_mode )                                  // Which mode (VOLUME, PRESET, TRACK)?
    {
      case VOLUME :
        if ( muteflag )
        {
          tftset ( 3, "" ) ;                                  // Clear text
        }
        else
        {
          tftset ( 3, "Mute" ) ;
        }
        muteflag = !muteflag ;                                // Mute/unmute
        break ;
      case PRESET :
        currentpreset = -1 ;                                  // Make sure current is different
        ini_block.newpreset = enc_preset ;                    // Make a definite choice
        enc_menu_mode = VOLUME ;                              // Back to default mode
        tftset ( 3, "" ) ;                                    // Clear text
        break ;
      case TRACK :
        host = enc_filename ;                                 // Selected track as new host
        hostreq = true ;                                      // Request this host
        enc_menu_mode = VOLUME ;                              // Back to default mode
        tftset ( 3, "" ) ;                                    // Clear text
        break ;
    }
  }
  if ( longclick )                                            // Check for long click
  {
    dbgprint ( "Long click") ;
    if ( datamode != STOPPED )
    {
      setdatamode ( STOPREQD ) ;                              // Request STOP, do not touch logclick flag
    }
    else
    {
      longclick = false ;                                     // Reset condition
      dbgprint ( "Long click detected" ) ;
      if ( get_FS_nodecount() )                               // Tracks on FS?
      {
        dbgprint ( "getFSfilename random choice" ) ;
        host = getFSfilename ( rt ) ;                         // Get random track
        hostreq = true ;                                      // Request this host
      }
      muteflag = false ;                                      // Be sure muteing is off
    }
  }
  if ( rotationcount == 0 )                                   // Any rotation?
  {
    return ;                                                  // No, return
  }
  dbgprint ( "Rotation count %d", rotationcount ) ;
  switch ( enc_menu_mode )                                    // Which mode (VOLUME, PRESET, TRACK)?
  {
    case VOLUME :
      if ( ( ini_block.reqvol + rotationcount ) < 0 )         // Limit volume
      {
        ini_block.reqvol = 0 ;                                // Limit to normal values
      }
      else if ( ( ini_block.reqvol + rotationcount ) > 100 )
      {
        ini_block.reqvol = 100 ;                              // Limit to normal values
      }
      else
      {
        ini_block.reqvol += rotationcount ;
      }
      muteflag = false ;                                      // Mute off
      break ;
    case PRESET :
      if ( ( enc_preset + rotationcount ) < 0 )               // Negative not allowed
      {
        enc_preset = 0 ;                                      // Stay at 0
      }
      else
      {
        enc_preset += rotationcount ;                         // Next preset
      }
      tmp = readhostfrompref ( enc_preset ) ;                 // Get host spec and possible comment
      if ( tmp == "" )                                        // End of presets?
      {
        enc_preset = 0 ;                                      // Yes, wrap
        tmp = readhostfrompref ( enc_preset ) ;               // Get host spec and possible comment
      }
      dbgprint ( "Preset is %d", enc_preset ) ;
      // Show just comment if available.  Otherwise the preset itself.
      inx = tmp.indexOf ( "#" ) ;                             // Get position of "#"
      if ( inx > 0 )                                          // Hash sign present?
      {
        tmp.remove ( 0, inx + 1 ) ;                           // Yes, remove non-comment part
      }
      chomp ( tmp ) ;                                         // Remove garbage from description
      tftset ( 3, tmp ) ;                                     // Set screen segment bottom part
      break ;
    case TRACK :
      enc_nodeID = selectnextFSnode ( rotationcount ) ;       // Select the next file on SD/USB
      enc_filename = getFSfilename ( enc_nodeID ) ;           // Set new filename
      tmp = enc_filename ;                                    // Copy for display
      dbgprint ( "Select %s", tmp.c_str() ) ;
      while ( ( inx = tmp.indexOf ( "/" ) ) >= 0 )            // Search for last slash
      {
        tmp.remove ( 0, inx + 1 ) ;                           // Remove before the slash
      }
      dbgprint ( "Simplified %s", tmp.c_str() ) ;
      tftset ( 3, tmp ) ;
    // Set screen segment bottom part
    default :
      break ;
  }
  rotationcount = 0 ;                                         // Reset
}


//**************************************************************************************************
//                                           M P 3 L O O P                                         *
//**************************************************************************************************
// Called from the mail loop() for the mp3 functions.                                              *
// A connection to an MP3 server is active and we are ready to receive data.                       *
// Normally there is about 2 to 4 kB available in the data stream.  This depends on the sender.    *
//**************************************************************************************************
void mp3loop()
{
  uint32_t        maxchunk ;                             // Max number of bytes to read
  int             res = 0 ;                              // Result reading from mp3 stream
  uint32_t        av = 0 ;                               // Available in stream
  String          nodeID ;                               // Next nodeID of track on SD
  uint32_t        timing ;                               // Startime and duration this function
  uint32_t        qspace ;                               // Free space in data queue

  // Try to keep the Queue to playtask filled up by adding as much bytes as possible
  if ( datamode & ( INIT | HEADER | DATA |               // Test op playing
                    METADATA | PLAYLISTINIT |
                    PLAYLISTHEADER |
                    PLAYLISTDATA ) )
  {
    timing = millis() ;                                  // Start time this function
    maxchunk = sizeof(tmpbuff) ;                         // Reduce byte count for this mp3loop()
    qspace = uxQueueSpacesAvailable( dataqueue ) *       // Compute free space in data queue
             sizeof(qdata_struct) ;
    if ( localfile )                                     // Playing file from SD card or USB drive?
    {
      av = mp3filelength ;                               // Bytes left in file
      if ( av < maxchunk )                               // Reduce byte count for this mp3loop()
      {
        maxchunk = av ;
      }
      if ( maxchunk > qspace )                           // Enough space in queue?
      {
        maxchunk = qspace ;                              // No, limit to free queue space
      }
      if ( maxchunk )                                    // Anything to read?
      {
        claimSPI ( "fsread" ) ;                          // Claim SPI bus
        res = read_FS ( tmpbuff, maxchunk ) ;            // Read a block of data
        releaseSPI() ;                                   // Release SPI bus
        mp3filelength -= res ;                           // Number of bytes left
      }
    }
    else
    {
      av = mp3client.available() ;                       // Available from stream
      if ( av < maxchunk )                               // Limit read size
      {
        maxchunk = av ;
      }
      if ( maxchunk > qspace )                           // Enough space in queue?
      {
        maxchunk = qspace ;                              // No, limit to free queue space
      }
      if ( maxchunk )                                    // Anything to read?
      {
        res = mp3client.read ( tmpbuff, maxchunk ) ;     // Read a number of bytes from the stream
        if (maxchunk > max_mp3chunk)                     // New maximum for chunksize found?
        {
          max_mp3chunk = maxchunk ;                      // Yes, set new maximum
          dbgprint ( "Chunkmax mp3loop %d", maxchunk ) ; // and report it
        }
        if ( av > max_mp3av )                            // New maximum for available on stream found?
        {
          max_mp3av = av ;                               // Yes, set new maximum
          dbgprint ( "Maxavail mp3loop %d", av ) ;       // and report it
          
        }
      }
    }
    if ( maxchunk == 0 )
    {
      if ( datamode == PLAYLISTDATA )                    // End of playlist
      {
        playlist_num = 1 ;                               // Yes, restart playlist
        dbgprint ( "End of playlist seen" ) ;
        setdatamode ( STOPPED ) ;
        ini_block.newpreset++ ;                          // Go to next preset
      }
    }
    for ( int i = 0 ; i < res ; i++ )
    {
      handlebyte_ch ( tmpbuff[i] ) ;                     // Handle one byte
    }
    timing = millis() - timing ;                         // Duration this function
    if ( timing > max_mp3loop_time )                     // New maximum found?
    {
      max_mp3loop_time = timing ;                        // Yes, set new maximum
      dbgprint ( "Duration mp3loop %d", timing ) ;       // and report it
    }
  }
  if ( datamode == STOPREQD )                            // STOP requested?
  {
    dbgprint ( "STOP requested" ) ;
    if ( localfile )
    {
      claimSPI ( "close" ) ;                             // Claim SPI bus
      close_SDCARD() ;
      releaseSPI() ;                                     // Release SPI bus
#if defined(RETRORADIO)
      localfile = false;
#endif
    }
    else
    {
      stop_mp3client() ;                                 // Disconnect if still connected
    }
    chunked = false ;                                    // Not longer chunked
    datacount = 0 ;                                      // Reset datacount
    outqp = outchunk.buf ;                               // and pointer
    queuefunc ( QSTOPSONG ) ;                            // Queue a request to stop the song
    metaint = 0 ;                                        // No metaint known now
    setdatamode ( STOPPED ) ;                            // Yes, state becomes STOPPED
    //Serial.println("Datamode STOPPED reached\r\n");
    return ;
  }
  if ( localfile )                                       // Playing from SD?
  {
    if ( datamode & DATA )                               // Test op playing
    {
      if ( av == 0 )                                     // End of mp3 data?
      {
//        Serial.println("End of mp3track");
        setdatamode ( STOPREQD ) ;                       // End of local mp3-file detected
#if defined(RETRORADIO)                                 // some radio stations play from playlist. that might confuse localfile.
        playlist_num = 0;                                // retroradio does not play playlists... 
#endif
        if ( playlist_num )                              // Playing from playlist?
        {
//          Serial.println("In PlaylistMode...");
          playlist_num++ ;                               // Yes, goto next item in playlist
          setdatamode ( PLAYLISTINIT ) ;
          host = playlist ;
        }
        else
        {
          nodeID = selectnextFSnode ( +1 ) ;             // Select the next file on SD/USB
//          Serial.printf("Select next from NodeId: %s\r\n", nodeID);
          host = getFSfilename ( nodeID ) ;
        }
        hostreq = true ;                                 // Request this host
      }
    }
  }
  if ( ini_block.newpreset != currentpreset )            // New station or next from playlist requested?
  {
    if ( datamode != STOPPED )                           // Yes, still busy?
    {
      setdatamode ( STOPREQD ) ;                         // Yes, request STOP
    }
    else
    {
      if ( playlist_num )                                 // Playing from playlist?
      { // Yes, retrieve URL of playlist
        playlist_num += ini_block.newpreset -
                        currentpreset ;                   // Next entry in playlist
        ini_block.newpreset = currentpreset ;             // Stay at current preset
      }
      else
      {
        host = readhostfrompref() ;                       // Lookup preset in preferences
        chomp ( host ) ;                                  // Get rid of part after "#"
      }
      dbgprint ( "New preset/file requested (%d/%d) from %s",
                 ini_block.newpreset, playlist_num, host.c_str() ) ;
      if ( host != ""  )                                  // Preset in ini-file?
      {
        hostreq = true ;                                  // Force this station as new preset
      }
      else
      {
        // This preset is not available, return to preset 0, will be handled in next mp3loop()
        dbgprint ( "No host for this preset" ) ;
        ini_block.newpreset = 0 ;                         // Wrap to first station
      }
    }
  }
  if ( hostreq )                                          // New preset or station?
  {
    hostreq = false ;
    currentpreset = ini_block.newpreset ;                 // Remember current preset
    mqttpub.trigger ( MQTT_PRESET ) ;                     // Request publishing to MQTT
    // Find out if this URL is on localhost (SD).
    localfile = ( host.indexOf ( "localhost/" ) >= 0 ) ;
    if ( localfile )                                      // Play file from localhost?
    {
      if ( ! connecttofile() )                            // Yes, open mp3-file
      {
        setdatamode ( STOPPED ) ;                         // Start in DATA mode
      }
    }
    else
    {
      if ( host.startsWith ( "ihr/" ) )                   // iHeartRadio station requested?
      {
        host = host.substring ( 4 ) ;                     // Yes, remove "ihr/"
        host = xmlgethost ( host ) ;                      // Parse the xml to get the host
      }
      connecttohost() ;                                   // Switch to new host
    }
  }
}


//**************************************************************************************************
//                                           L O O P                                               *
//**************************************************************************************************
// Main loop of the program.                                                                       *
//**************************************************************************************************
void loop()
{
  mp3loop() ;                                       // Do mp3 related actions
  if ( updatereq )                                  // Software update requested?
  {
    if ( displaytype == T_NEXTION )                 // NEXTION in use?
    { 
      update_software ( "lstmodn",                  // Yes, update NEXTION image from remote image
                        UPDATEHOST, TFTFILE ) ;
    }
    update_software ( "lstmods",                    // Update sketch from remote file
                      UPDATEHOST, BINFILE ) ;
    resetreq = true ;                               // And reset
  }
  if ( resetreq )                                   // Reset requested?
  {
    delay ( 1000 ) ;                                // Yes, wait some time
    ESP.restart() ;                                 // Reboot
  }
  scanserial() ;                                    // Handle serial input
  scanserial2() ;                                   // Handle serial input from NEXTION (if active)
  scandigital() ;                                   // Scan digital inputs
  scanIR() ;                                        // See if IR input
  ArduinoOTA.handle() ;                             // Check for OTA
  mp3loop() ;                                       // Do more mp3 related actions
  handlehttpreply() ;
  cmdclient = cmdserver.available() ;               // Check Input from client?
  if ( cmdclient )                                  // Client connected?
  {
    dbgprint ( "Command client available" ) ;
    handlehttp() ;
  }
  // Handle MQTT.
  if ( mqtt_on )
  {
    mqttclient.loop() ;                             // Handling of MQTT connection
  }
  handleSaveReq() ;                                 // See if time to save settings
  handleIpPub() ;                                   // See if time to publish IP
  handleVolPub() ;                                  // See if time to publish volume
#if defined RETRORADIO
  loopRetroRadio();
#else
  chk_enc() ;                                       // Check rotary encoder functions
  check_CH376() ;                                   // Check Flashdrive insert/remove
#endif
}


//**************************************************************************************************
//                                    C H K H D R L I N E                                          *
//**************************************************************************************************
// Check if a line in the header is a reasonable headerline.                                       *
// Normally it should contain something like "icy-xxxx:abcdef".                                    *
//**************************************************************************************************
bool chkhdrline ( const char* str )
{
  char    b ;                                         // Byte examined
  int     len = 0 ;                                   // Lengte van de string

  while ( ( b = *str++ ) )                            // Search to end of string
  {
    len++ ;                                           // Update string length
    if ( ! isalpha ( b ) )                            // Alpha (a-z, A-Z)
    {
      if ( b != '-' )                                 // Minus sign is allowed
      {
        if ( b == ':' )                               // Found a colon?
        {
          return ( ( len > 5 ) && ( len < 50 ) ) ;    // Yes, okay if length is okay
        }
        else
        {
          return false ;                              // Not a legal character
        }
      }
    }
  }
  return false ;                                      // End of string without colon
}


//**************************************************************************************************
//                            S C A N _ C O N T E N T _ L E N G T H                                *
//**************************************************************************************************
// If the line contains content-length information: set clength (content length counter).          *
//**************************************************************************************************
void scan_content_length ( const char* metalinebf )
{
  if ( strstr ( metalinebf, "Content-Length" ) )        // Line contains content length
  {
    clength = atoi ( metalinebf + 15 ) ;                // Yes, set clength
    dbgprint ( "Content-Length is %d", clength ) ;      // Show for debugging purposes
  }
}


//**************************************************************************************************
//                                   H A N D L E B Y T E _ C H                                     *
//**************************************************************************************************
// Handle the next byte of data from server.                                                       *
// Chunked transfer encoding aware. Chunk extensions are not supported.                            *
//**************************************************************************************************
void handlebyte_ch ( uint8_t b )
{
  static int       chunksize = 0 ;                      // Chunkcount read from stream
  static uint16_t  playlistcnt ;                        // Counter to find right entry in playlist
  static int       LFcount ;                            // Detection of end of header
  static bool      ctseen = false ;                     // First line of header seen or not

  if ( chunked &&
       ( datamode & ( DATA |                           // Test op DATA handling
                      METADATA |
                      PLAYLISTDATA ) ) )
  {
    if ( chunkcount == 0 )                             // Expecting a new chunkcount?
    {
      if ( b == '\r' )                                 // Skip CR
      {
        return ;
      }
      else if ( b == '\n' )                            // LF ?
      {
        chunkcount = chunksize ;                       // Yes, set new count
        chunksize = 0 ;                                // For next decode
        return ;
      }
      // We have received a hexadecimal character.  Decode it and add to the result.
      b = toupper ( b ) - '0' ;                        // Be sure we have uppercase
      if ( b > 9 )
      {
        b = b - 7 ;                                    // Translate A..F to 10..15
      }
      chunksize = ( chunksize << 4 ) + b ;
      return  ;
    }
    chunkcount-- ;                                     // Update count to next chunksize block
  }
  if ( datamode == DATA )                              // Handle next byte of MP3/Ogg data
  {
    *outqp++ = b ;
    if ( outqp == ( outchunk.buf + sizeof(outchunk.buf) ) ) // Buffer full?
    {
      // Send data to playtask queue.  If the buffer cannot be placed within 200 ticks,
      // the queue is full, while the sender tries to send more.  The chunk will be dis-
      // carded it that case.
      xQueueSend ( dataqueue, &outchunk, 200 ) ;       // Send to queue
      outqp = outchunk.buf ;                           // Item empty now
    }
    if ( metaint )                                     // No METADATA on Ogg streams or mp3 files
    {
      if ( --datacount == 0 )                          // End of datablock?
      {
        setdatamode ( METADATA ) ;
        metalinebfx = -1 ;                             // Expecting first metabyte (counter)
      }
    }
    return ;
  }
  if ( datamode == INIT )                              // Initialize for header receive
  {
    ctseen = false ;                                   // Contents type not seen yet
    metaint = 0 ;                                      // No metaint found
    LFcount = 0 ;                                      // For detection end of header
    bitrate = 0 ;                                      // Bitrate still unknown
    dbgprint ( "Switch to HEADER" ) ;
    setdatamode ( HEADER ) ;                           // Handle header
    totalcount = 0 ;                                   // Reset totalcount
    metalinebfx = 0 ;                                  // No metadata yet
    metalinebf[0] = '\0' ;
  }
  if ( datamode == HEADER )                            // Handle next byte of MP3 header
  {
    if ( ( b > 0x7F ) ||                               // Ignore unprintable characters
         ( b == '\r' ) ||                              // Ignore CR
         ( b == '\0' ) )                               // Ignore NULL
    {
      // Yes, ignore
    }
    else if ( b == '\n' )                              // Linefeed ?
    {
      LFcount++ ;                                      // Count linefeeds
      metalinebf[metalinebfx] = '\0' ;                 // Take care of delimiter
      if ( chkhdrline ( metalinebf ) )                 // Reasonable input?
      {
        dbgprint ( "Headerline: %s",                   // Show headerline
                   metalinebf ) ;
        String metaline = String ( metalinebf ) ;      // Convert to string
        String lcml = metaline ;                       // Use lower case for compare
        lcml.toLowerCase() ;
        if ( lcml.startsWith ( "location: http://" ) ) // Redirection?
        {
          host = metaline.substring ( 17 ) ;           // Yes, get new URL
          hostreq = true ;                             // And request this one
        }
        if ( lcml.indexOf ( "content-type" ) >= 0)     // Line with "Content-Type: xxxx/yyy"
        {
          ctseen = true ;                              // Yes, remember seeing this
          String ct = metaline.substring ( 13 ) ;      // Set contentstype. Not used yet
          ct.trim() ;
          dbgprint ( "%s seen.", ct.c_str() ) ;
        }
        if ( lcml.startsWith ( "icy-br:" ) )
        {
          bitrate = metaline.substring(7).toInt() ;    // Found bitrate tag, read the bitrate
          if ( bitrate == 0 )                          // For Ogg br is like "Quality 2"
          {
            bitrate = 87 ;                             // Dummy bitrate
          }
        }
        else if ( lcml.startsWith ("icy-metaint:" ) )
        {
          metaint = metaline.substring(12).toInt() ;   // Found metaint tag, read the value
        }
        else if ( lcml.startsWith ( "icy-name:" ) )
        {
          icyname = metaline.substring(9) ;            // Get station name
          icyname.trim() ;                             // Remove leading and trailing spaces
          tftset ( 2, icyname ) ;                      // Set screen segment bottom part
          mqttpub.trigger ( MQTT_ICYNAME ) ;           // Request publishing to MQTT
        }
        else if ( lcml.startsWith ( "transfer-encoding:" ) )
        {
          // Station provides chunked transfer
          if ( lcml.endsWith ( "chunked" ) )
          {
            chunked = true ;                           // Remember chunked transfer mode
            chunkcount = 0 ;                           // Expect chunkcount in DATA
          }
        }
      }
      metalinebfx = 0 ;                                // Reset this line
      if ( ( LFcount == 2 ) && ctseen )                // Content type seen and a double LF?
      {
        dbgprint ( "Switch to DATA, bitrate is %d"     // Show bitrate
                   ", metaint is %d",                  // and metaint
                   bitrate, metaint ) ;
        setdatamode ( DATA ) ;                         // Expecting data now
        datacount = metaint ;                          // Number of bytes before first metadata
        queuefunc ( QSTARTSONG ) ;                     // Queue a request to start song
      }
    }
    else
    {
      metalinebf[metalinebfx++] = (char)b ;            // Normal character, put new char in metaline
      if ( metalinebfx >= METASIZ )                    // Prevent overflow
      {
        metalinebfx-- ;
      }
      LFcount = 0 ;                                    // Reset double CRLF detection
    }
    return ;
  }
  if ( datamode == METADATA )                          // Handle next byte of metadata
  {
    if ( metalinebfx < 0 )                             // First byte of metadata?
    {
      metalinebfx = 0 ;                                // Prepare to store first character
      metacount = b * 16 + 1 ;                         // New count for metadata including length byte
      if ( metacount > 1 )
      {
        dbgprint ( "Metadata block %d bytes",
                   metacount - 1 ) ;                   // Most of the time there are zero bytes of metadata
      }
    }
    else
    {
      metalinebf[metalinebfx++] = (char)b ;            // Normal character, put new char in metaline
      if ( metalinebfx >= METASIZ )                    // Prevent overflow
      {
        metalinebfx-- ;
      }
    }
    if ( --metacount == 0 )
    {
      metalinebf[metalinebfx] = '\0' ;                 // Make sure line is limited
      if ( strlen ( metalinebf ) )                     // Any info present?
      {
        // metaline contains artist and song name.  For example:
        // "StreamTitle='Don McLean - American Pie';StreamUrl='';"
        // Sometimes it is just other info like:
        // "StreamTitle='60s 03 05 Magic60s';StreamUrl='';"
        // Isolate the StreamTitle, remove leading and trailing quotes if present.
        showstreamtitle ( metalinebf ) ;               // Show artist and title if present in metadata
        mqttpub.trigger ( MQTT_STREAMTITLE ) ;         // Request publishing to MQTT
      }
      if ( metalinebfx  > ( METASIZ - 10 ) )           // Unlikely metaline length?
      {
        dbgprint ( "Metadata block too long! Skipping all Metadata from now on." ) ;
        metaint = 0 ;                                  // Probably no metadata
      }
      datacount = metaint ;                            // Reset data count
      //bufcnt = 0 ;                                   // Reset buffer count
      setdatamode ( DATA ) ;                           // Expecting data
    }
  }
  if ( datamode == PLAYLISTINIT )                      // Initialize for receive .m3u file
  {
    // We are going to use metadata to read the lines from the .m3u file
    // Sometimes this will only contain a single line
    metalinebfx = 0 ;                                  // Prepare for new line
    LFcount = 0 ;                                      // For detection end of header
    if ( localfile )                                   // SD-card mode?
    {
      setdatamode ( PLAYLISTDATA ) ;                   // Yes, no header here
    }
    else
    {
      setdatamode ( PLAYLISTHEADER ) ;                 // Handle playlist header
    }
    playlistcnt = 1 ;                                  // Reset for compare
    totalcount = 0 ;                                   // Reset totalcount
    clength = 0xFFFFFFFF ;                             // Content-length unknown
    dbgprint ( "Read from playlist" ) ;
  }
  if ( datamode == PLAYLISTHEADER )                    // Read header
  {
    if ( ( b > 0x7F ) ||                               // Ignore unprintable characters
         ( b == '\r' ) ||                              // Ignore CR
         ( b == '\0' ) )                               // Ignore NULL
    {
      return ;                                         // Quick return
    }
    else if ( b == '\n' )                              // Linefeed ?
    {
      LFcount++ ;                                      // Count linefeeds
      metalinebf[metalinebfx] = '\0' ;                 // Take care of delimeter
      dbgprint ( "Playlistheader: %s",                 // Show playlistheader
                 metalinebf ) ;
      scan_content_length ( metalinebf ) ;             // Check if it is a content-length line
      metalinebfx = 0 ;                                // Ready for next line
      if ( LFcount == 2 )
      {
        dbgprint ( "Switch to PLAYLISTDATA, "          // For debug
                   "search for entry %d",
                   playlist_num ) ;
        setdatamode ( PLAYLISTDATA ) ;                 // Expecting data now
        mqttpub.trigger ( MQTT_PLAYLISTPOS ) ;         // Playlistposition to MQTT
        return ;
      }
    }
    else
    {
      metalinebf[metalinebfx++] = (char)b ;            // Normal character, put new char in metaline
      if ( metalinebfx >= METASIZ )                    // Prevent overflow
      {
        metalinebfx-- ;
      }
      LFcount = 0 ;                                    // Reset double CRLF detection
    }
  }
  if ( datamode == PLAYLISTDATA )                      // Read next byte of .m3u file data
  {
    clength-- ;                                        // Decrease content length by 1
    if ( ( b > 0x7F ) ||                               // Ignore unprintable characters
         ( b == '\r' ) ||                              // Ignore CR
         ( b == '\0' ) )                               // Ignore NULL
    {
      // Yes, ignore
    }
    if ( b != '\n' )                                   // Linefeed?
    { // No, normal character in playlistdata,
      metalinebf[metalinebfx++] = (char)b ;            // add it to metaline
      if ( metalinebfx >= METASIZ )                    // Prevent overflow
      {
        metalinebfx-- ;
      }
    }
    if ( ( b == '\n' ) ||                              // linefeed ?
         ( clength == 0 ) )                            // Or end of playlist data contents
    {
      int inx ;                                        // Pointer in metaline
      metalinebf[metalinebfx] = '\0' ;                 // Take care of delimeter
      dbgprint ( "Playlistdata: %s",                   // Show playlistheader
                 metalinebf ) ;
      if ( strlen ( metalinebf ) < 5 )                 // Skip short lines
      {
        metalinebfx = 0 ;                              // Flush line
        metalinebf[0] = '\0' ;
        return ;
      }
      String metaline = String ( metalinebf ) ;        // Convert to string
      if ( metaline.indexOf ( "#EXTINF:" ) >= 0 )      // Info?
      {
        if ( playlist_num == playlistcnt )             // Info for this entry?
        {
          inx = metaline.indexOf ( "," ) ;             // Comma in this line?
          if ( inx > 0 )
          {
            // Show artist and title if present in metadata
            showstreamtitle ( metaline.substring ( inx + 1 ).c_str(), true ) ;
            mqttpub.trigger ( MQTT_STREAMTITLE ) ;     // Request publishing to MQTT
          }
        }
      }
      if ( metaline.startsWith ( "#" ) )               // Commentline?
      {
        metalinebfx = 0 ;                              // Yes, ignore
        return ;                                       // Ignore commentlines
      }
      // Now we have an URL for a .mp3 file or stream.  Is it the rigth one?
      dbgprint ( "Entry %d in playlist found: %s", playlistcnt, metalinebf ) ;
      if ( playlist_num == playlistcnt  )
      {
        inx = metaline.indexOf ( "http://" ) ;         // Search for "http://"
        if ( inx >= 0 )                                // Does URL contain "http://"?
        {
          host = metaline.substring ( inx + 7 ) ;      // Yes, remove it and set host
        }
        else
        {
          host = metaline ;                            // Yes, set new host
        }
        if ( localfile )                               // SD card mode?
        {
          if ( ! metaline.startsWith ( "localhost" ) ) // Prepend "localhost" if missing
          {
            host = String ( "localhost/" ) + metaline ;
          }
          if ( ! connecttofile() )                     // Yes, connect to file
          {
            setdatamode ( STOPPED ) ;                  // Error, stop!
          }
        }
        else
        {
          connecttohost() ;                           // Connect to stream host
        }
      }
      metalinebfx = 0 ;                                // Prepare for next line
      host = playlist ;                                // Back to the .m3u host
      playlistcnt++ ;                                  // Next entry in playlist
    }
  }
}


//**************************************************************************************************
//                                     G E T C O N T E N T T Y P E                                 *
//**************************************************************************************************
// Returns the contenttype of a file to send.                                                      *
//**************************************************************************************************
String getContentType ( String filename )
{
  if      ( filename.endsWith ( ".html" ) ) return "text/html" ;
  else if ( filename.endsWith ( ".png"  ) ) return "image/png" ;
  else if ( filename.endsWith ( ".gif"  ) ) return "image/gif" ;
  else if ( filename.endsWith ( ".jpg"  ) ) return "image/jpeg" ;
  else if ( filename.endsWith ( ".ico"  ) ) return "image/x-icon" ;
  else if ( filename.endsWith ( ".css"  ) ) return "text/css" ;
  else if ( filename.endsWith ( ".zip"  ) ) return "application/x-zip" ;
  else if ( filename.endsWith ( ".gz"   ) ) return "application/x-gzip" ;
  else if ( filename.endsWith ( ".mp3"  ) ) return "audio/mpeg" ;
  else if ( filename.endsWith ( ".pw"   ) ) return "" ;              // Passwords are secret
  return "text/plain" ;
}


//**************************************************************************************************
//                                        H A N D L E F S F                                        *
//**************************************************************************************************
// Handling of requesting pages from the PROGMEM. Example: favicon.ico                             *
//**************************************************************************************************
void handleFSf ( const String& pagename )
{
  String                 ct ;                           // Content type
  const char*            p ;
  int                    l ;                            // Size of requested page
  int                    TCPCHUNKSIZE = 1024 ;          // Max number of bytes per write

  dbgprint ( "FileRequest received %s", pagename.c_str() ) ;
  ct = getContentType ( pagename ) ;                    // Get content type
  if ( ( ct == "" ) || ( pagename == "" ) )             // Empty is illegal
  {
    cmdclient.println ( "HTTP/1.1 404 Not Found" ) ;
    cmdclient.println ( "" ) ;
    return ;
  }
  else
  {
    if ( pagename.indexOf ( "index.html" ) >= 0 )       // Index page is in PROGMEM
    {
      p = index_html ;
      l = sizeof ( index_html ) ;
    }
    else if ( pagename.indexOf ( "radio.css" ) >= 0 )   // CSS file is in PROGMEM
    {
      p = radio_css + 1 ;
      l = sizeof ( radio_css ) ;
    }
    else if ( pagename.indexOf ( "config.html" ) >= 0 ) // Config page is in PROGMEM
    {
      p = config_html ;
      l = sizeof ( config_html ) ;
    }
    else if ( pagename.indexOf ( "mp3play.html" ) >= 0 ) // Mp3player page is in PROGMEM
    {
      p = mp3play_html ;
      l = sizeof ( mp3play_html ) ;
    }
    else if ( pagename.indexOf ( "about.html" ) >= 0 )  // About page is in PROGMEM
    {
      p = about_html ;
      l = sizeof ( about_html ) ;
    }
    else if ( pagename.indexOf ( "favicon.ico" ) >= 0 ) // Favicon icon is in PROGMEM
    {
      p = (char*)favicon_ico ;
      l = sizeof ( favicon_ico ) ;
    }
    else
    {
      p = index_html ;
      l = sizeof ( index_html ) ;
    }
    if ( *p == '\n' )                                   // If page starts with newline:
    {
      p++ ;                                             // Skip first character
      l-- ;
    }
    dbgprint ( "Length of page is %d", strlen ( p ) ) ;
    cmdclient.print ( httpheader ( ct ) ) ;             // Send header
    // The content of the HTTP response follows the header:
    if ( l < 10 )
    {
      cmdclient.println ( "Testline<br>" ) ;
    }
    else
    {
      while ( l )                                       // Loop through the output page
      {
        if ( l <= TCPCHUNKSIZE )                        // Near the end?
        {
          cmdclient.write ( p, l ) ;                    // Yes, send last part
          l = 0 ;
        }
        else
        {
          cmdclient.write ( p, TCPCHUNKSIZE ) ;         // Send part of the page
          p += TCPCHUNKSIZE ;                           // Update startpoint and rest of bytes
          l -= TCPCHUNKSIZE ;
        }
      }
    }
    // The HTTP response ends with another blank line:
    cmdclient.println() ;
    dbgprint ( "Response send" ) ;
  }
}


//**************************************************************************************************
//                                         C H O M P                                               *
//**************************************************************************************************
// Do some filtering on de inputstring:                                                            *
//  - String comment part (starting with "#").                                                     *
//  - Strip trailing CR.                                                                           *
//  - Strip leading spaces.                                                                        *
//  - Strip trailing spaces.                                                                       *
//**************************************************************************************************
void chomp ( String &str )
{
  int   inx ;                                         // Index in de input string

  if ( ( inx = str.indexOf ( "#" ) ) >= 0 )           // Comment line or partial comment?
  {
    str.remove ( inx ) ;                              // Yes, remove
  }
  str.trim() ;                                        // Remove spaces and CR
}

#if defined(RETRORADIO)
//**************************************************************************************************
//                                    C H O M P _ N V S                                            *
//**************************************************************************************************
// Do some filtering on de inputstring:                                                            *
//  - do 'normal' chomp first, return if resulting string does not start with @                    *
//  - if resulting string starts with '@', nvs is looked if a key with the name following @ is     *
//    found in nvs. If so, that string (chomped) is returned or empty if key is not found          *
//**************************************************************************************************
void chomp_nvs ( String &str , const char* substitute)
{
  //Serial.printf("Chomp NVS with %s\r\n", str.c_str());Serial.flush();
  if (substitute) {
    int idx = str.indexOf('?');
    while(idx >= 0) {
      str = str.substring(0, idx) + String(substitute) + str.substring(idx + 1);
      idx = str.indexOf('?');
    }
  }
  chomp ( str ) ;                                     // Normal chomp first
  if (str.c_str()[0] == '@' )                         // Reference to NVS-Key?
  {
    if ( nvssearch (str.c_str() + 1 ) ) 
    { 
      //Serial.printf("NVS Search success for key: %s, result =", str.c_str() + 1);
      str = nvsgetstr ( str.c_str() + 1) ;
      chomp ( str ) ;
      //Serial.println(str);
    }
    else if ( ramsearch (str.c_str() + 1 ) ) 
    { 
      //Serial.printf("NVS Search success for key: %s\r\n", str.c_str() + 1);
      str = ramgetstr ( str.c_str() + 1) ;
      chomp ( str ) ;
    } 
    else
      str = "";   
  }
}
#endif


//**************************************************************************************************
//                                   A N A L Y Z E C M D S                                         *
//**************************************************************************************************
// Handling of the various commands from remote webclient, Serial or MQTT.                         *
// Here a sequence of commands is allowed, commands are expected to be seperated by ';'            *   
//**************************************************************************************************
const char* analyzeCmds ( String commands )
{
  static char reply[512] ;                       // Reply to client, will be returned
  uint16_t commands_executed = 0 ;               // How many commands have been executed  

  char * cmds = strdup ( commands.c_str() ) ;
  if ( cmds ) 
  {
    char *s = cmds ;
    while ( s ) 
    { 
      char *p = s;
      char * next = NULL;
      int nesting = 0;
      while (*p && !next) {
        if (*p == '(')
          nesting++;
        else if (nesting == 0) {
          if (*p == ';' ) 
            next = p;
        } else if (*p == ')')
          nesting--;
        p++;
      }
      if ( next ) 
      {
        *next = 0 ;
        next++ ;
      }
      analyzeCmd ( s ) ;
      commands_executed++ ;
      s = next ;
    }
    free ( cmds ) ;
  }
  snprintf ( reply, sizeof ( reply ), "Executed %d command(s) from sequence %s", commands_executed, commands.c_str() ) ;
  return reply ;
}


//**************************************************************************************************
//                                     A N A L Y Z E C M D                                         *
//**************************************************************************************************
// Handling of the various commands from remote webclient, Serial or MQTT.                         *
// Version for handling string with: <parameter>=<value>                                           *
//**************************************************************************************************
const char* analyzeCmd ( const char* str )
{
  char*        value ;                           // Points to value after equalsign in command
  const char*  res ;                             // Result of analyzeCmd

  value = strstr ( str, "=" ) ;                  // See if command contains a "="
  if ( value )
  {
    *value = '\0' ;                              // Separate command from value
    res = analyzeCmd ( str, value + 1 ) ;        // Analyze command and handle it
    *value = '=' ;                               // Restore equal sign
  }
  else
  {
    res = analyzeCmd ( str, "0" ) ;              // No value, assume zero
  }
  return res ;
}


//**************************************************************************************************
//                                     A N A L Y Z E C M D                                         *
//**************************************************************************************************
// Handling of the various commands from remote webclient, serial or MQTT.                         *
// par holds the parametername and val holds the value.                                            *
// "wifi_00" and "preset_00" may appear more than once, like wifi_01, wifi_02, etc.                *
// Examples with available parameters:                                                             *
//   preset     = 12                        // Select start preset to connect to                   *
//   preset_00  = <mp3 stream>              // Specify station for a preset 00-max *)              *
//   volume     = 95                        // Percentage between 0 and 100                        *
//   upvolume   = 2                         // Add percentage to current volume                    *
//   downvolume = 2                         // Subtract percentage from current volume             *
//   toneha     = <0..15>                   // Setting treble gain                                 *
//   tonehf     = <0..15>                   // Setting treble frequency                            *
//   tonela     = <0..15>                   // Setting bass gain                                   *
//   tonelf     = <0..15>                   // Setting treble frequency                            *
//   station    = <mp3 stream>              // Select new station (will not be saved)              *
//   station    = <URL>.mp3                 // Play standalone .mp3 file (not saved)               *
//   station    = <URL>.m3u                 // Select playlist (will not be saved)                 *
//   stop                                   // Stop playing                                        *
//   resume                                 // Resume playing                                      *
//   mute                                   // Mute/unmute the music (toggle)                      *
//   wifi_00    = mySSID/mypassword         // Set WiFi SSID and password *)                       *
//   mqttbroker = mybroker.com              // Set MQTT broker to use *)                           *
//   mqttprefix = XP93g                     // Set MQTT broker to use                              *
//   mqttport   = 1883                      // Set MQTT port to use, default 1883 *)               *
//   mqttuser   = myuser                    // Set MQTT user for authentication *)                 *
//   mqttpasswd = mypassword                // Set MQTT password for authentication *)             *
//   clk_server = pool.ntp.org              // Time server to be used *)                           *
//   clk_offset = <-11..+14>                // Offset with respect to UTC in hours *)              *
//   clk_dst    = <1..2>                    // Offset during daylight saving time in hours *)      *
//   mp3track   = <nodeID>                  // Play track from SD card, nodeID 0 = random          *
//   settings                               // Returns setting like presets and tone               *
//   status                                 // Show current URL to play                            *
//   test                                   // For test purposes                                   *
//   debug      = 0 or 1                    // Switch debugging on or off                          *
//   reset                                  // Restart the ESP32                                   *
//   bat0       = 2318                      // ADC value for an empty battery                      *
//   bat100     = 2916                      // ADC value for a fully charged battery               *
//   fs         = USB or SD                 // Select local filesystem for MP# player mode.        *
//  Commands marked with "*)" are sensible during initialization only                              *
//**************************************************************************************************
const char* analyzeCmd ( const char* par, const char* val )
{
  String             argument ;                       // Argument as string
  String             value ;                          // Value of an argument as a string
  int                ivalue ;                         // Value of argument as an integer
  static char        reply[180] ;                     // Reply to client, will be returned
  uint8_t            oldvol ;                         // Current volume
  bool               relative ;                       // Relative argument (+ or -)
  String             tmpstr ;                         // Temporary for value
  uint32_t           av ;                             // Available in stream/file

  blset ( true ) ;                                    // Enable backlight of TFT
  strcpy ( reply, "Command accepted" ) ;              // Default reply
  argument = String ( par ) ;                         // Get the argument
  chomp ( argument ) ;                                // Remove comment and useless spaces
  if ( argument.length() == 0 )                       // Lege commandline (comment)?
  {
    return reply ;                                    // Ignore
  }
  argument.toLowerCase() ;                            // Force to lower case
  value = String ( val ) ;                            // Get the specified value
#if defined(RETRORADIO)
  chomp_nvs ( value ) ;                               // Remove comment and extra spaces
#else
  chomp ( value );
#endif  
  ivalue = value.toInt() ;                            // Also as an integer
  ivalue = abs ( ivalue ) ;                           // Make positive
  relative = argument.indexOf ( "up" ) == 0 ;         // + relative setting?
  if ( argument.indexOf ( "down" ) == 0 )             // - relative setting?
  {
    relative = true ;                                 // It's relative
    ivalue = - ivalue ;                               // But with negative value
  }
  if ( value.startsWith ( "http://" ) )               // Does (possible) URL contain "http://"?
  {
    value.remove ( 0, 7 ) ;                           // Yes, remove it
  }
  if ( value.length() )
  {
    tmpstr = value ;                                  // Make local copy of value
    if ( argument.indexOf ( "passw" ) >= 0 )          // Password in value?
    {
      tmpstr = String ( "*******" ) ;                 // Yes, hide it
    }
    dbgprint ( "Command: %s with parameter %s",
               argument.c_str(), tmpstr.c_str() ) ;
  }
  else
  {
    dbgprint ( "Command: %s (without parameter)",
               argument.c_str() ) ;
  }
#if defined(RETRORADIO)
  if (argument == "execute") {    
/*    Serial.println("Running Execute!");Serial.flush();
   dbgprint ( "Command: %s with parameter %s",
               argument.c_str(), tmpstr.c_str() ) ;
    Serial.println("Next Showing tmpstr!?");Serial.flush();
    Serial.printf("tmpstr.length()=%d\r\n", tmpstr.length());Serial.flush();
    Serial.printf("Tmpstr = \"%s\"\r\n", tmpstr.c_str());Serial.flush();
    Serial.println("Next Showing Value!?");Serial.flush();
    Serial.printf("Value.length()=%d\r\n", value.length());Serial.flush();
    Serial.printf("Value = \"%s\"\r\n", value.c_str());Serial.flush();
    chomp(value);
    Serial.printf("ChompedValue = \"%s\"\r\n", value.c_str());Serial.flush();
    value.toLowerCase();
    Serial.printf("lowercaseValue = \"%s\"\r\n", value.c_str());Serial.flush();
    Serial.printf("Execute command from nvs[\"%s\"]=%s\r\n", value.c_str(), nvsgetstr(value.c_str()).c_str());
*/
    chomp(value);
    analyzeCmds(nvssearch(value.c_str())?nvsgetstr(value.c_str()):ramgetstr(value.c_str()));  
  } else if ((argument == "ramlist") || (argument == "nvslist")) {
    const char* p = NULL;
    value.toLowerCase();
    chomp(value);
    if (value  != "0")
      p = value.c_str();
    if (argument.c_str()[0] == 'r')
      doRamlist(p);
    else
      doNvslist(p);
  }else if (argument == "inlist")  {
    const char* p = NULL;
    value.toLowerCase();
    chomp(value);
    if (value != "0")
      p = value.c_str();
    Serial.printf("inlist found with value'%s', length=%d, resulting p=%ld\r\n", value.c_str(), value.length(), p);
    doInlist(p);
  } 
  
  else 
#endif    // RETRORADIO  
  
  if ( argument.indexOf ( "volume" ) >= 0 )           // Volume setting?
  {
    // Volume may be of the form "upvolume", "downvolume" or "volume" for relative or absolute setting
    // Or minimum ("volume_min") or maximum ("vol_max") or Zerfo-Flag ("volume_zero") setting
    oldvol = vs1053player->getVolume() ;              // Get current volume
    if ( relative )                                   // + relative setting?
    {
      ini_block.reqvol = oldvol + ivalue ;            // Up/down by 0.5 or more dB
    }
#ifdef RETRORADIO
    else if (argument.indexOf ( "_max" ) > 0 )
    {
      ini_block.vol_max = ivalue;
    }
    else if (argument.indexOf ( "_min" ) > 0 )
    {
      ini_block.vol_min = ivalue;
//      Serial.printf ("\r\n\r\nMinvol=%d\r\n", ini_block.vol_min);
    }
/*
    else if (argument.indexOf ("_zero") > 0 )
    {
      if ( value == "0" )
        ini_block.vol_zero = 0;
      else
        ini_block.vol_zero = 1;
    }
*/
#endif
    else
    {
      ini_block.reqvol = ivalue ;                     // Absolue setting
    }
#ifdef RETRORADIO
    if (( ini_block.reqvol > 127 ) ||                 // Wrapped around?
        (ini_block.reqvol < ini_block.vol_min))  // or below defined minimum?                        
    { 
        if (!relative || (ivalue < 0))                  // coming from above defined minimum?
          ini_block.reqvol = 0 ;                          // Yes, go to zero
        else
          ini_block.reqvol = ini_block.vol_min;
    }
    if ( ini_block.reqvol > ini_block.vol_max )
    {
      ini_block.reqvol = ini_block.vol_max ;                        // Limit to normal values
    }
#else
    if ( ini_block.reqvol > 127 )                     // Wrapped around?
    {
      ini_block.reqvol = 0 ;                          // Yes, keep at zero
    }
    if ( ini_block.reqvol > 100 )
    {
      ini_block.reqvol = 100 ;                        // Limit to normal values
    }
#endif    
    muteflag = false ;                                // Stop possibly muting

    sprintf ( reply, "Volume is now %d",              // Reply new volume
              ini_block.reqvol ) ;
//    Serial.printf ("\r\n\r\n%s\r\n", reply);
  }
  else if ( argument == "mute" )                      // Mute/unmute request
  {
    muteflag = !muteflag ;                            // Request volume to zero/normal
  }
  else if ( argument.indexOf ( "ir_" ) >= 0 )         // Ir setting?
  { // Do not handle here
  }
  else if ( argument.indexOf ( "preset_" ) >= 0 )     // Enumerated preset?
  { // Do not handle here
  }
  else if ( argument.indexOf ( "preset" ) >= 0 )      // (UP/DOWN)Preset station?
  {
    // If MP3 player is active: change track
    if ( localfile &&
         ( ( datamode & DATA ) != 0 ) &&              // MP# player active?
         relative )
    {
      setdatamode ( STOPREQD ) ;                      // Force stop MP3 player
      if ( playlist_num )                             // In playlist mode?
      {
        playlist_num += ivalue ;                      // Set new entry number
        if ( playlist_num <= 0 )                      // Limit number
        {
          playlist_num = 1 ;
        }
        host = playlist ;                             // Yes, prepare to read playlist
      }
      else
      {
        tmpstr = selectnextFSnode ( ivalue ) ;        // Select the next or previous file on SD/USB
        host = getFSfilename ( tmpstr ) ;
        sprintf ( reply, "Playing %s",                // Reply new filename
                  host.c_str() ) ;
      }
      hostreq = true ;                                // Request this host
    }
    else
    {
/*
#if defined(RETRORADIO)
      chomp(value);
      value.toLowerCase();
      if ( value == "any" ) {
         selectRandomPreset();
      } else 
#endif
*/
      if ( relative )                                 // Relative argument?
      {
        currentpreset = ini_block.newpreset ;         // Remember currentpreset
        ini_block.newpreset += ivalue ;               // Yes, adjust currentpreset
      }
      else
      {
        ini_block.newpreset = ivalue ;                // Otherwise set station
        playlist_num = 0 ;                            // Absolute, reset playlist
        currentpreset = -1 ;                          // Make sure current is different
      }
      setdatamode ( STOPREQD ) ;                      // Force stop MP3 player
      sprintf ( reply, "Preset is now %d",            // Reply new preset
                ini_block.newpreset ) ;
    }
  }
  else if ( argument == "stop" )                      // (un)Stop requested?
  {
    if ( datamode & ( HEADER | DATA | METADATA | PLAYLISTINIT |
                      PLAYLISTHEADER | PLAYLISTDATA ) )

    {
      setdatamode ( STOPREQD ) ;                      // Request STOP
    }
    else
    {
      hostreq = true ;                                // Request UNSTOP
    }
  }
  else if ( ( value.length() > 0 ) &&
            ( ( argument == "mp3track" ) ||           // Select a track from SD card?
              ( argument == "station" ) ) )           // Station in the form address:port
  {
    if ( argument.startsWith ( "mp3" ) )              // MP3 track to search for
    {
      value = getFSfilename ( value ) ;               // like "localhost/........"
      if ( value.length() == 0 )                      // Found?
      {
        strcpy ( reply, "Command not accepted!" ) ;   // No, error reply
        return reply ;
      }
    }
    if ( datamode & ( HEADER | DATA | METADATA | PLAYLISTINIT |
                      PLAYLISTHEADER | PLAYLISTDATA ) )
    {
      setdatamode ( STOPREQD ) ;                      // Request STOP
    }
    host = value ;                                    // Save it for storage and selection later
    hostreq = true ;                                  // Force this station as new preset
    sprintf ( reply,
              "Playing %s",                           // Format reply
              host.c_str() ) ;
    utf8ascii_ip ( reply ) ;                          // Remove possible strange characters
  }
  else if ( argument == "status" )                    // Status request
  {
    if ( datamode == STOPPED )
    {
      sprintf ( reply, "Player stopped" ) ;           // Format reply
    }
    else
    {
      sprintf ( reply, "%s - %s", icyname.c_str(),
                icystreamtitle.c_str() ) ;            // Streamtitle from metadata
    }
  }
  else if ( argument.startsWith ( "reset" ) )         // Reset request
  {
    resetreq = true ;                                 // Reset all
  }
  else if ( argument.startsWith ( "update" ) )        // Update request
  {
    updatereq = true ;                                // Reset all
  }
  else if ( argument == "test" )                      // Test command
  {
    if ( localfile )
    {
      av = mp3filelength ;                            // Available bytes in file
    }
    else
    {
      av = mp3client.available() ;                    // Available in stream
    }
    sprintf ( reply, "Free memory is %d, chunks in queue %d, stream %d, bitrate %d kbps",
              ESP.getFreeHeap(),
              uxQueueMessagesWaiting ( dataqueue ),
              av,
              mbitrate ) ;
    dbgprint ( "Stack maintask is %d", uxTaskGetStackHighWaterMark ( maintask ) ) ;
    dbgprint ( "Stack playtask is %d", uxTaskGetStackHighWaterMark ( xplaytask ) ) ;
    dbgprint ( "Stack spftask  is %d", uxTaskGetStackHighWaterMark ( xspftask ) ) ;
    dbgprint ( "ADC reading is %d", adcval ) ;
    dbgprint ( "scaniocount is %d", scaniocount ) ;
    dbgprint ( "Max. mp3_loop duration is %d", max_mp3loop_time ) ;
#if defined(RETRORADIO)    
    dbgprint ( "Max. mp3_chunk size is %d", max_mp3chunk ) ;
    dbgprint ( "Max. mp3 available was %d", max_mp3av ) ;
    max_mp3chunk = 0;
    max_mp3av = 0;
    nvs_stats_t nvs_stats;
    nvs_get_stats(NULL, &nvs_stats);
    dbgprint ("NVS-Count: UsedEntries = (%d), FreeEntries = (%d), AllEntries = (%d)",
              nvs_stats.used_entries, nvs_stats.free_entries, nvs_stats.total_entries);  
    dbgprint ("Total PSRAM: %d", ESP.getPsramSize());
    dbgprint ("Free PSRAM: %d", ESP.getFreePsram());
#endif       
    max_mp3loop_time = 0 ;                            // Start new check
  }
  // Commands for bass/treble control
  else if ( argument.startsWith ( "tone" ) )          // Tone command
  {
    if ( argument.indexOf ( "ha" ) > 0 )              // High amplitue? (for treble)
    {
      ini_block.rtone[0] = ivalue ;                   // Yes, prepare to set ST_AMPLITUDE
    }
    if ( argument.indexOf ( "hf" ) > 0 )              // High frequency? (for treble)
    {
      ini_block.rtone[1] = ivalue ;                   // Yes, prepare to set ST_FREQLIMIT
    }
    if ( argument.indexOf ( "la" ) > 0 )              // Low amplitue? (for bass)
    {
      ini_block.rtone[2] = ivalue ;                   // Yes, prepare to set SB_AMPLITUDE
    }
    if ( argument.indexOf ( "lf" ) > 0 )              // High frequency? (for bass)
    {
      ini_block.rtone[3] = ivalue ;                   // Yes, prepare to set SB_FREQLIMIT
    }
    reqtone = true ;                                  // Set change request
    sprintf ( reply, "Parameter for bass/treble %s set to %d",
              argument.c_str(), ivalue ) ;
  }
  else if ( argument == "rate" )                      // Rate command?
  {
    vs1053player->AdjustRate ( ivalue ) ;             // Yes, adjust
  }
  else if ( argument.startsWith ( "mqtt" ) )          // Parameter fo MQTT?
  {
    strcpy ( reply, "MQTT broker parameter changed. Save and restart to have effect" ) ;
    if ( argument.indexOf ( "broker" ) > 0 )          // Broker specified?
    {
      ini_block.mqttbroker = value ;                  // Yes, set broker accordingly
    }
    else if ( argument.indexOf ( "prefix" ) > 0 )     // Port specified?
    {
      ini_block.mqttprefix = value ;                  // Yes, set port user accordingly
    }
    else if ( argument.indexOf ( "port" ) > 0 )       // Port specified?
    {
      ini_block.mqttport = ivalue ;                   // Yes, set port user accordingly
    }
    else if ( argument.indexOf ( "user" ) > 0 )       // User specified?
    {
      ini_block.mqttuser = value ;                    // Yes, set user accordingly
    }
    else if ( argument.indexOf ( "passwd" ) > 0 )     // Password specified?
    {
      ini_block.mqttpasswd = value.c_str() ;          // Yes, set broker password accordingly
    }
  }
  else if ( argument == "debug" )                     // debug on/off request?
  {
    DEBUG = ivalue ;                                  // Yes, set flag accordingly
  }
  else if ( argument == "btdebug" )                   // debug on/off request for BlueTooth?
  {
    DEBUG = ivalue ;                                  // Yes, set flag accordingly
  }
  else if ( argument == "getnetworks" )               // List all WiFi networks?
  {
    sprintf ( reply, networks.c_str() ) ;             // Reply is SSIDs
  }
  else if ( argument.startsWith ( "clk_" ) )          // TOD parameter?
  {
    if ( argument.indexOf ( "server" ) > 0 )          // Yes, NTP server spec?
    {
      ini_block.clk_server = value ;                  // Yes, set server
    }
    if ( argument.indexOf ( "offset" ) > 0 )          // Offset with respect to UTC spec?
    {
      ini_block.clk_offset = value.toInt() ;          // Yes, set offset
    }
    if ( argument.indexOf ( "dst" ) > 0 )             // Offset duringe DST spec?
    {
      ini_block.clk_dst = value.toInt() ;             // Yes, set DST offset
    }
  }
  else if ( argument.startsWith ( "bat" ) )           // Battery ADC value?
  {
    if ( argument.indexOf ( "100" ) == 3 )            // 100 percent value?
    {
      ini_block.bat100 = ivalue ;                     // Yes, set it
    }
    else if ( argument.indexOf ( "0" ) == 3 )         // 0 percent value?
    {
      ini_block.bat0 = ivalue ;                       // Yes, set it
    }
  }
  else if ( argument == "fs" )                        // Filesystem for MP3 player mode?
  {
    if ( value.equalsIgnoreCase ( "usb" ) )           // Yes, is it USB?
    {
      usb_sd = FS_USB ;                               // Yes, change FS setting to USB
    }
    else
    {
      usb_sd = FS_SD ;                                // Otherwise to SD
    }
  }
#if defined(RETRORADIO)
  else if ( argument == "settings" ) {
    strncpy(reply, getradiostatus().c_str(), 179);
//  } else if ( argument == "eq" ) {
//    setEqualizer (value, ivalue);
//    strcpy(reply, "OK");
  } else if ( argument == "channels" ) {
    doChannels (value);
    strcpy(reply, "OK");
  } else if ( argument == "channel" ) {
    doChannel (value, ivalue);
    strcpy(reply, "OK");
/*
  } else if ( argument.startsWith ( "rr_") ) {
    strcpy(reply, retroradioSetup(argument, value, ivalue).c_str());
  } else if ( argument == "calibrate" ) {
    setCalibration(value);
  } else if ( argument == "usetunings") {
    useTunings(ivalue);
  }  else if ( argument == "random" ) {
    doRandomPlay();
  } else if ( argument == "togglehost" ) {
    doToggleHost((bool)ivalue);
  } else if ( argument == "channel" ) {
    doChannel(value, ivalue);
  } else if ( argument.startsWith( "led" )) {
     int idx = argument.c_str()[3] - '0';
     if ((idx < 0) || (idx > 10))
      idx = 0;
     doLed(idx, value);
*/
  } else if ( argument.startsWith("nvs" ) || argument.startsWith ("ram" )) {
    if (argument.c_str()[3] == '.') {
      bool isNvs = (argument.c_str()[0] == 'n');
      argument = argument.substring(4);
      chomp(argument);
      if ( isNvs )
          nvssetstr(argument.c_str(), value);  
      else
          ramsetstr(argument.c_str(), value);  
    } else {
      int idx = value.indexOf('=');
      if (idx > 0) {
        argument = value.substring(0, idx);
        value = value.substring(idx + 1);
        argument.toLowerCase();
        chomp(argument);
        chomp(value);
        if ( argument == "nvs" )
          nvssetstr(argument.c_str(), value);  
        else
          ramsetstr(argument.c_str(), value);  
      }
    }  
/*    
  } else if (argument == "randmode") {
      doRandMode(value);
  } else if (argument == "nop") {
      ; // relax: nothing to do!!!!
  } else if (argument == "lock") {
      chomp(value);
      value.toLowerCase();
      doLockHmi(value, ivalue); 
  } else if (argument == "lockvol") {
      chomp(value);
      value.toLowerCase();
      doLockVol(value, ivalue); 
*/  
  } else if (argument.startsWith("in.")) {
    doInput(argument.substring(3), value);
  } else if (argument.startsWith("if")) {
    doIf(value, argument.c_str()[2] == 'v');
  } else if (argument.startsWith("calc")) {
    doCalc(value, argument.c_str()[4] == 'v');
  }
/*  
  else if (argument == "input") {
    doInput(value);
  } else if (argument == "read") {
    doRead(value);                                //TBD: Debug only
  }
*/
#endif
#if defined(TRACKLIST)
  else if (argument == "tracklist") {
    value.toLowerCase();
    if (SD_okay) {
      if (value == "0") {
        sprintf(reply, "Tracklist \"%s\" is %sused.", SD_tracklistname, (SD_hastracklist?"":"not "));
      } else if (value == "delete") {
        if (!SD_hastracklist) 
          sprintf(reply, "Tracklist \"%s\" does not exist!", SD_tracklistname);
        else {
          bool deleteSuccess;
          claimSPI ( "command" );
          deleteSuccess = SD.remove( SD_tracklistname );
          releaseSPI ();
          if (SD_hastracklist = !deleteSuccess) 
            sprintf(reply, "Failed to delete tracklist \"%s\".", SD_tracklistname);
          else
            sprintf(reply, "Tracklist \"%s\" deleted.", SD_tracklistname);
        }
      } else if (value == "init") {
        if (SD_hastracklist) 
          sprintf(reply, "Tracklist \"%s\" does exist. Run command \"tracklist=delete\" first to delete it.", SD_tracklistname);
        else {
          File f;
          claimSPI ( "command" );
          f = SD.open( SD_tracklistname, FILE_WRITE );
          f.close();
          releaseSPI ();         
          sprintf(reply, "Empty Tracklist \"%s\" created. Will be filled at next start of radio.", SD_tracklistname);
           
        }
      }
    }
    else {
      strcpy(reply, "SD-card not available!");
    }
  }    
#endif

  else
  {
    sprintf ( reply, "%s called with illegal parameter: %s",
              NAME, argument.c_str() ) ;
  }
  return reply ;                                      // Return reply to the caller
}


//**************************************************************************************************
//                                     H T T P H E A D E R                                         *
//**************************************************************************************************
// Set http headers to a string.                                                                   *
//**************************************************************************************************
String httpheader ( String contentstype )
{
  return String ( "HTTP/1.1 200 OK\nContent-type:" ) +
         contentstype +
         String ( "\n"
                  "Server: " NAME "\n"
                  "Cache-Control: " "max-age=3600\n"
                  "Last-Modified: " VERSION "\n\n" ) ;
}


//**************************************************************************************************
//* Function that are called from spftask.                                                         *
//* Note that some device dependent function are place in the *.h files.                           *
//**************************************************************************************************

//**************************************************************************************************
//                                      D I S P L A Y I N F O                                      *
//**************************************************************************************************
// Show a string on the LCD at a specified y-position (0..2) in a specified color.                 *
// The parameter is the index in tftdata[].                                                        *
void displayinfo ( uint16_t inx )
{
  uint16_t       width = dsp_getwidth() ;                  // Normal number of colums
  scrseg_struct* p = &tftdata[inx] ;
  uint16_t len ;                                           // Length of string, later buffer length

  if ( inx == 0 )                                          // Topline is shorter
  {
    width += TIMEPOS ;                                     // Leave space for time
  }
  if ( tft )                                               // TFT active?
  {
    dsp_fillRect ( 0, p->y, width, p->height, BLACK ) ;    // Clear the space for new info
    if ( ( dsp_getheight() > 64 ) && ( p->y > 1 ) )        // Need and space for divider?
    {
      dsp_fillRect ( 0, p->y - 4, width, 1, GREEN ) ;      // Yes, show divider above text
    }
    len = p->str.length() ;                                // Required length of buffer
    if ( len++ )                                           // Check string length, set buffer length
    {
      char buf [ len ] ;                                   // Need some buffer space
      p->str.toCharArray ( buf, len ) ;                    // Make a local copy of the string
      utf8ascii_ip ( buf ) ;                               // Convert possible UTF8
      dsp_setTextColor ( p->color ) ;                      // Set the requested color
      dsp_setCursor ( 0, p->y ) ;                          // Prepare to show the info
      dsp_println ( buf ) ;                                // Show the string
    }
  }
}


//**************************************************************************************************
//                                         G E T T I M E                                           *
//**************************************************************************************************
// Retrieve the local time from NTP server and convert to string.                                  *
// Will be called every second.                                                                    *
//**************************************************************************************************
void gettime()
{
  static int16_t delaycount = 0 ;                           // To reduce number of NTP requests
  static int16_t retrycount = 100 ;

  if ( tft )                                                // TFT used?
  {
    if ( timeinfo.tm_year )                                 // Legal time found?
    {
      sprintf ( timetxt, "%02d:%02d:%02d",                  // Yes, format to a string
                timeinfo.tm_hour,
                timeinfo.tm_min,
                timeinfo.tm_sec ) ;
    }
    if ( --delaycount <= 0 )                                // Sync every few hours
    {
      delaycount = 7200 ;                                   // Reset counter
      if ( timeinfo.tm_year )                               // Legal time found?
      {
        dbgprint ( "Sync TOD, old value is %s", timetxt ) ;
      }
      dbgprint ( "Sync TOD" ) ;
      if ( !getLocalTime ( &timeinfo ) )                    // Read from NTP server
      {
        dbgprint ( "Failed to obtain time!" ) ;             // Error
        timeinfo.tm_year = 0 ;                              // Set current time to illegal
        if ( retrycount )                                   // Give up syncing?
        {
          retrycount-- ;                                    // No try again
          delaycount = 5 ;                                  // Retry after 5 seconds
        }
      }
      else
      {
        sprintf ( timetxt, "%02d:%02d:%02d",                // Format new time to a string
                  timeinfo.tm_hour,
                  timeinfo.tm_min,
                  timeinfo.tm_sec ) ;
        dbgprint ( "Sync TOD, new value is %s", timetxt ) ;
      }
    }
  }
}


//**************************************************************************************************
//                                H A N D L E _ T F T _ T X T                                      *
//**************************************************************************************************
// Check if tft refresh is requested.                                                              *
//**************************************************************************************************
bool handle_tft_txt()
{
  for ( uint16_t i = 0 ; i < TFTSECS ; i++ )              // Handle all sections
  {
    if ( tftdata[i].update_req )                          // Refresh requested?
    {
      displayinfo ( i ) ;                                 // Yes, do the refresh
      dsp_update() ;                                      // Updates to the screen
      tftdata[i].update_req = false ;                     // Reset request
      return true ;                                       // Just handle 1 request
    }
  }
  return false ;                                          // Not a single request
}


//**************************************************************************************************
//                                     P L A Y T A S K                                             *
//**************************************************************************************************
// Play stream data from input queue.                                                              *
// Handle all I/O to VS1053B during normal playing.                                                *
// Handles display of text, time and volume on TFT as well.                                        *
//**************************************************************************************************
void playtask ( void * parameter )
{
  while ( true )
  {
    if ( xQueueReceive ( dataqueue, &inchunk, 5 ) )
    {
      while ( !vs1053player->data_request() )                       // If FIFO is full..
      {
        vTaskDelay ( 1 ) ;                                          // Yes, take a break
      }
      switch ( inchunk.datatyp )                                    // What kind of chunk?
      {
        case QDATA:
          claimSPI ( "chunk" ) ;                                    // Claim SPI bus
          vs1053player->playChunk ( inchunk.buf,                    // DATA, send to player
                                    sizeof(inchunk.buf) ) ;
          releaseSPI() ;                                            // Release SPI bus
          totalcount += sizeof(inchunk.buf) ;                       // Count the bytes
          break ;
        case QSTARTSONG:
          playingstat = 1 ;                                         // Status for MQTT
          mqttpub.trigger ( MQTT_PLAYING ) ;                        // Request publishing to MQTT
          claimSPI ( "startsong" ) ;                                // Claim SPI bus
          vs1053player->startSong() ;                               // START, start player
          releaseSPI() ;                                            // Release SPI bus
          break ;
        case QSTOPSONG:
          playingstat = 0 ;                                         // Status for MQTT
          mqttpub.trigger ( MQTT_PLAYING ) ;                        // Request publishing to MQTT
          claimSPI ( "stopsong" ) ;                                 // Claim SPI bus
          vs1053player->setVolume ( 0 ) ;                           // Mute
          vs1053player->stopSong() ;                                // STOP, stop player
          releaseSPI() ;                                            // Release SPI bus
          while ( xQueueReceive ( dataqueue, &inchunk, 0 ) ) ;      // Flush rest of queue
          vTaskDelay ( 500 / portTICK_PERIOD_MS ) ;                 // Pause for a short time
          break ;
        default:
          break ;
      }
    }
    //esp_task_wdt_reset() ;                                        // Protect against idle cpu
  }
  //vTaskDelete ( NULL ) ;                                          // Will never arrive here
}


//**************************************************************************************************
//                                   H A N D L E _ S P E C                                         *
//**************************************************************************************************
// Handle special (non-stream data) functions for spftask.                                         *
//**************************************************************************************************
void handle_spec()
{
  // Do some special function if necessary
  if ( dsp_usesSPI() )                                        // Does display uses SPI?
  {
    claimSPI ( "hspectft" ) ;                                 // Yes, claim SPI bus
  }
  if ( tft )                                                  // Need to update TFT?
  {
    handle_tft_txt() ;                                        // Yes, TFT refresh necessary
    dsp_update() ;                                            // Be sure to paint physical screen
  }
  if ( dsp_usesSPI() )                                        // Does display uses SPI?
  {
    releaseSPI() ;                                            // Yes, release SPI bus
  }
  if ( time_req && NetworkFound )                             // Time to refresh time?
  {
    gettime() ;                                               // Yes, get the current time
  }
  claimSPI ( "hspec" ) ;                                      // Claim SPI bus
  if ( muteflag )                                             // Mute or not?
  {
    vs1053player->setVolume ( 0 ) ;                           // Mute
  }
  else
  {
    vs1053player->setVolume ( ini_block.reqvol ) ;            // Unmute
  }
  if ( reqtone )                                              // Request to change tone?
  {
    reqtone = false ;
    vs1053player->setTone ( ini_block.rtone ) ;               // Set SCI_BASS to requested value
  }
  if ( time_req )                                             // Time to refresh timetxt?
  {
    time_req = false ;                                        // Yes, clear request
    if ( NetworkFound  )                                      // Time available?
    {
      displaytime ( timetxt ) ;                               // Write to TFT screen
      displayvolume() ;                                       // Show volume on display
      displaybattery() ;                                      // Show battery charge on display
    }
  }
  releaseSPI() ;                                              // Release SPI bus
  if ( mqtt_on )
  {
    if ( !mqttclient.connected() )                            // See if connected
    {
      mqttreconnect() ;                                       // No, reconnect
    }
    else
    {
      mqttpub.publishtopic() ;                                // Check if any publishing to do
    }
  }
}


//**************************************************************************************************
//                                     S P F T A S K                                               *
//**************************************************************************************************
// Handles display of text, time and volume on TFT.                                                *
// Handles ADC meassurements.                                                                      *
// This task runs on a low priority.                                                               *
//**************************************************************************************************
void spftask ( void * parameter )
{
  while ( true )
  {
    handle_spec() ;                                                 // Maybe some special funcs?
    vTaskDelay ( 100 / portTICK_PERIOD_MS ) ;                       // Pause for a short time
    adcval = ( 15 * adcval +                                        // Read ADC and do some filtering
               adc1_get_raw ( ADC1_CHANNEL_0 ) ) / 16 ;
  }
  //vTaskDelete ( NULL ) ;                                          // Will never arrive here
}