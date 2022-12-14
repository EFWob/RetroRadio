#ifndef RETRORADIOEXTENSION_H__
#define RETRORADIOEXTENSION_H__
#include <Arduino.h>
#include <nvs.h>
#include <memory>
#include <PubSubClient.h>
#include <FS.h>
#include <SD.h>
#include <time.h>
#ifndef NVS_KEY_NAME_MAX_SIZE
#define NVS_KEY_NAME_MAX_SIZE 16
#endif
#define ANNOUNCEMODE_RUNDOWN ((uint8_t)0x80)
#define ANNOUNCEMODE_NOCANCEL ((uint8_t)0x40)
#define ANNOUNCEMODE_PRIO0    ((uint8_t)0x1)
#define ANNOUNCEMODE_PRIO1    ((uint8_t)0x2)
#define ANNOUNCEMODE_PRIO2    ((uint8_t)0x4)
#define ANNOUNCEMODE_PRIO3    ((uint8_t)0x8)
#define ANNOUNCEMODE_PRIOALL  (ANNOUNCEMODE_PRIO0 | ANNOUNCEMODE_PRIO1 | ANNOUNCEMODE_PRIO2 | ANNOUNCEMODE_PRIO3)  


#define UP_AND_RUNNING() (NULL != xplaytask)
#define IDENT_SPECIALCHARS   "$_:"
#define isIdentchar(c)    (isalnum(c) || (NULL != strchr(IDENT_SPECIALCHARS, c)))
extern bool isIdentifier(const char *s);


//extern void retroRadioInit();
extern void setupRR(uint8_t setupLevel);
extern void loopRR();
//extern const char* analyzeCmdsRR ( const char* commands );
extern const char* analyzeCmdsRR ( String commands );
extern void httpHandleGenre ( String http_rqfile, String http_getcmd );
extern String readfavfrompref ( int16_t idx );


#if defined(NORETRORADIO)
#warning "NORETRORADIO IS DEFINED!"
#endif
#if (NORETRORADIO != 1)
#ifndef NAME
#define RADIONAME "ESP32Radio"
#else
#define ST(A) #A
#define STR(A) ST(A)
#define RADIONAME STR(NAME)
#endif
#define RETRORADIO
#ifndef ETHERNET

#define ETHERNET 2  // Set to '0' if you do not want Ethernet support at all
                    // Set to 1 to compile with Ethernet support
                    // Set to 2 to compile with Ethernet depending on board setting
                    //      (works for Olimex POE and most likely Olimex POE ISO)

#endif
#define SETUP_START 0
#define SETUP_NET 1
#define SETUP_DONE 2




#if defined(DEBUG_BUFFER_SIZE)
#undef DEBUG_BUFFER_SIZE
#endif
#if defined(NVSBUFSIZE)
#undef NVSBUFSIZE
#endif
#if defined(MAXKEYS)
#undef MAXKEYS
#endif

#define DEBUG_BUFFER_SIZE 500
#define NVSBUFSIZE 500
//#define MAXKEYS 500


struct ini_struct
{
  String         mqttbroker ;                         // The name of the MQTT broker server
  String         mqttprefix ;                         // Prefix to use for topics
  uint16_t       mqttport ;                           // Port, default 1883
  String         mqttuser ;                           // User for MQTT authentication
  String         mqttpasswd ;                         // Password for MQTT authentication
  uint16_t       mqttdelay ;                          // minimum delay (ms) between MQTT-messages
  uint8_t        reqvol ;                             // Requested volume
  uint8_t        rtone[4] ;                           // Requested bass/treble settings
  int16_t        newpreset ;                          // Requested preset
  String         clk_server ;                         // Server to be used for time of day clock
  int8_t         clk_offset ;                         // Offset in hours with respect to UTC
  int8_t         clk_dst ;                            // Number of hours shift during DST
  int8_t         ir_pin ;                             // GPIO connected to output of IR decoder
  int8_t         bt_pin ;                             // GPIO connected check for user request of Bluetooth (startup)
  int8_t         bt_auto ;                            // If set to != 0, auto connect to avaiable BT-device
  int8_t         bt_off ;                             // Timeout for reset in BT mode to restart radio
  int8_t         bt_vol ;                             // Volume setting for BT mode
  int8_t         enc_clk_pin ;                        // GPIO connected to CLK of rotary encoder
  int8_t         enc_dt_pin ;                         // GPIO connected to DT of rotary encoder
  int8_t         enc_sw_pin ;                         // GPIO connected to SW of rotary encoder
  int8_t         tft_cs_pin ;                         // GPIO connected to CS of TFT screen
  int8_t         tft_dc_pin ;                         // GPIO connected to D/C or A0 of TFT screen
  int8_t         tft_scl_pin ;                        // GPIO connected to SCL of i2c TFT screen
  int8_t         tft_sda_pin ;                        // GPIO connected to SDA of I2C TFT screen
  int8_t         tft_bl_pin ;                         // GPIO to activate BL of display
  int8_t         tft_blx_pin ;                        // GPIO to activate BL of display (inversed logic)
  int8_t         sd_cs_pin ;                          // GPIO connected to CS of SD card
  int8_t         vs_cs_pin ;                          // GPIO connected to CS of VS1053
  int8_t         vs_dcs_pin ;                         // GPIO connected to DCS of VS1053
  int8_t         vs_dreq_pin ;                        // GPIO connected to DREQ of VS1053
  int8_t         vs_shutdown_pin ;                    // GPIO to shut down the amplifier
  int8_t         vs_shutdownx_pin ;                   // GPIO to shut down the amplifier (inversed logic)
  int8_t         spi_sck_pin ;                        // GPIO connected to SPI SCK pin
  int8_t         spi_miso_pin ;                       // GPIO connected to SPI MISO pin
  int8_t         spi_mosi_pin ;                       // GPIO connected to SPI MOSI pin
  int8_t         ch376_cs_pin ;                       // GPIO connected to CH376 SS
  int8_t         ch376_int_pin ;                      // GPIO connected to CH376 INT
  uint16_t       bat0 ;                               // ADC value for 0 percent battery charge
  uint16_t       bat100 ;                             // ADC value for 100 percent battery charge
  int8_t         espnowmode ;                         // current ESP-Now-Mode
  int8_t         espnowmodetarget ;                   // to store request if espnow not yet available
} ;

struct progpin_struct                                    // For programmable input pins
{
  int8_t         gpio ;                                  // Pin number
  bool           reserved ;                              // Reserved for connected devices
  bool           avail ;                                 // Pin is available for a command
  String         command ;                               // Command to execute when activated
                                                         // Example: "uppreset=1"
  bool           cur ;                                   // Current state, true = HIGH, false = LOW
} ;

struct touchpin_struct                                   // For programmable input pins
{
  int8_t         gpio ;                                  // Pin number GPIO
  bool           reserved ;                              // Reserved for connected devices
  bool           avail ;                                 // Pin is available for a command
  String         command ;                               // Command to execute when activated
  bool           cur ;                                   // Current state, true = HIGH, false = LOW
  int16_t        count ;                                 // Counter number of times low level
} ;

/*
struct keyname_t                                      // For keys in NVS
{
  char      Key[16] ;                                 // Max length is 15 plus delimeter
} ;
*/


#include <SPI.h>

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
      SPI.endTransaction() ;                      // Allow ovoid        claimSPI ( const char* p ) ;                // Claim SPI bus for exclusive access
void        releaseSPI() ;                              // Release the claimther SPI users
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
    uint32_t clock() {return VS1053_SPI._clock;} ;       // Get current clock setting of SPI for VS1053.
                                                         // Returns 0 if not initialized
                                                         // 200000 in slow mode, above if in fast mode

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

class UploadFile {
public:
  UploadFile() {};
  bool begin(String path, bool writeFlag = true);
  void close();
  operator bool() {return _isOpen;};
protected:
  inline void claim_spi();  
  inline void release_spi();
private:
  bool _isOpen = false;
  bool _isSD = false;
  bool _littleFSBegun = false;
  File _soundFile;
};

// prototypes for functions/global data in main.cpp()
extern int8_t            connecttohoststat ;                    // Status of connecttohost
extern UploadFile        uploadfile ;                           // File to upload alarm/alert sound to...
extern String            host;                                  // host to connect to
extern String            lasthost;                              // last successfully connected host
extern String            bootmode ;
extern bool              hostreq ;                              // Request for new host
extern bool              muteflag ;
extern uint8_t           announceMode ;                         // Announcement mode...
extern uint32_t          announceStart ;                        // when has announceMode started?
extern int               connectDelay ;                         // Station with ConnectDelay 
extern uint32_t          streamDelay ;
//extern CommandReply      commandReply ;                         // Container for command reply/replies


extern String            connectcmds ;                          // command(s) to be executed at (successfull) host connect
extern String            currentStation ;                       // URL [optional #name] of last host request (not chomped)
extern String            stationBefore;
extern String            ipaddress ;                            // Own IP-address
extern String            icyname ;                              // Icecast station name
extern String            icystreamtitle ;                       // Streamtitle from metadata
extern String            knownstationname ;                     // Name of station in prefs/genrelist
extern String            knownstationnamebefore ;               // To store original value in alert-mode

extern bool              NetworkFound ;                         // True if WiFi network connected
extern ini_struct        ini_block ;                            // Holds configurable data
extern VS1053*           vs1053player ;                         // The object for the MP3 player
//extern char              nvskeys[MAXKEYS][16] ;                 // Space for NVS keys
extern int               DEBUG ;                                // Debug on/off
extern int               DEBUGMQTT ;                            // MQTT Debbug on/off
extern progpin_struct    progpin[] ;                            // Input pins and programmed function
extern uint32_t          nvshandle  ;                           // Handle for nvs access
extern uint32_t          ir_0 ;                                 // Average duration of an IR short pulse
extern uint32_t          ir_1 ;                                 // Average duration of an IR long pulse
extern touchpin_struct   touchpin[] ;                           // Touch pins and programmed function
extern TaskHandle_t      maintask ;                             // Taskhandle for main task
extern TaskHandle_t      xplaytask ;                            // Task handle for playtask
extern TaskHandle_t      xspftask ;                             // Task handle for special functions
extern uint8_t           gmaintain ;                            // Genre-Maintenance-mode? (play is suspended)
extern volatile int16_t           currentpreset ;                        // Preset station playing
extern int               mqttfavidx;                            // idx of favorite info to publish on MQTT
extern int               mqttfavendidx;                         // last idx of favorite info to publish on MQTT
//extern std::vector<keyname_t> keynames ;                        // Keynames in NVS
extern std::vector<const char *> keynames ;                        // Keynames in NVS
extern std::vector<const char *> mqttpub_backlog ;              // backlog of MQTT-Messages to send
extern std::vector<const char *> mqttrcv_backlog ;              // Backlog for MQTT-messages received
extern std::vector<String>       cmd_backlog ;                  // Backlog for Commands to be executed
extern uint8_t           namespace_ID ;                         // Namespace ID found
extern bool              resetreq ;                             // Request to reset the ESP32
extern PubSubClient      mqttclient ;
extern bool              mqtt_on;
extern void              mqttInputBegin() ;
extern void        claimSPI ( const char* p ) ;                // Claim SPI bus for exclusive access
extern void        releaseSPI() ;                              // Release the claim
extern char              timetxt[9] ;                           // Converted timeinfo

char*       dbgprint( const char* format, ... ) ;
void        tftlog ( const char *str ) ;
void        chomp ( String &str ) ;
void        chomp_nvs ( String &str ) ;

esp_err_t   nvsclear ( ) ;
String      nvsgetstr ( const char* key, bool uncomment = true ) ;
int         nvssearch ( const char* key ) ;
esp_err_t   nvssetstr ( const char* key, String val ) ;
void        nvsdelkey ( const char* k);



void bubbleSortKeys ( std::vector<const char*>& keynames, uint16_t n );
//void        fillkeylist() ;
void fillkeylist(std::vector<const char*>& keynames, uint8_t namespaceid);
void fillnslist(std::vector<const char *>& namespaces);
const char* analyzeCmd ( const char* str ) ;
const char* analyzeCmd ( const char* par, const char* val ) ;
void reservepin ( int8_t rpinnr ) ;
bool connecttohost();
void mqttpubFavNotPlaying();
bool setAnnouncemode(uint8_t mode);
class CommandReply {
  public:
    CommandReply() {};
    ~CommandReply() {clear();};
    void begin() {clear();};
    void print();
    void invlidate() {_sane = false;};
    bool add(const char* s);
    bool vadd ( const char* format, ... );
  protected:
    void clear();
    bool addPage();
    std::vector<char *>_content;
    const size_t _pageSize = 1024;
    bool _sane = true;
    size_t _size = 0;
};

bool isAnnouncemode();
/*
void favplayreport(String url);
void favplayrequestinfo(String url, bool rescan = false);
*/
String getFavoriteJson(int idx, int rMin=1, int rMax=100);
void setLastStation(String last);
void scanFavorite();

extern struct tm         timeinfo ;                             // Will be filled by NTP server
extern volatile bool     minuteflag;                            // Will be set if minute has elapsed
extern void gettime();


//AddRR.cpp
String ramgetstr ( const char* key ) ;
void ramsetstr ( const char* key, String val ) ;
bool ramsearch ( const char* key ) ;
void ramdelkey ( const char* key) ;
const char* analyzeCmdsFromKey ( const char * key ) ;
const char* analyzeCmdsFromKey ( String key ) ;
extern String extractgroup(String& inValue);
extern int parseGroup(String & inValue, String& left, String& right, const char** delimiters = NULL, bool extendNvs = false,
        bool extendNvsLeft = false, bool extendNvsRight = false);



#endif
#endif