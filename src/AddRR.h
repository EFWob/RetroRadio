#ifndef RETRORADIOEXTENSION_H__
#define RETRORADIOEXTENSION_H__
#include <Arduino.h>

//extern void retroRadioInit();
extern void setupRR(uint8_t setupLevel);
extern void loopRR();
extern const char* analyzeCmdRR ( const char* commands );

#if defined(NORETRORADIO)
#warning "NORETRORADIO IS DEFINED!"
#endif
#if (NORETRORADIO != 1)
#define NAME "ESP32Radio"
#define RETRORADIO
#define ETHERNET 2  // Set to '0' if you do not want Ethernet support at all
                    // Set to 1 to compile with Ethernet support
                    // Set to 2 to compile with Ethernet depending on board setting
                    //      (works for Olimex POE and most likely Olimex POE ISO)
#define SETUP_START 0
#define SETUP_NET 1
#define SETUP_DONE 2





#define DEBUG_BUFFER_SIZE 500
#define NVSBUFSIZE 500
#define MAXKEYS 300


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
  int8_t         ir_pin ;                             // GPIO connected to output of IR decoder
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

// prototypes for functions/global data in main.cpp()
extern String            ipaddress ;                            // Own IP-address
extern bool              NetworkFound ;                         // True if WiFi network connected
extern ini_struct        ini_block ;                            // Holds configurable data
extern VS1053*           vs1053player ;                         // The object for the MP3 player
extern char              nvskeys[MAXKEYS][16] ;                 // Space for NVS keys
extern int               DEBUG ;                                // Debug on/off
extern progpin_struct    progpin[] ;                            // Input pins and programmed function
extern uint32_t          nvshandle  ;                           // Handle for nvs access
extern uint32_t          ir_0 ;                                 // Average duration of an IR short pulse
extern uint32_t          ir_1 ;                                 // Average duration of an IR long pulse
extern touchpin_struct   touchpin[] ;                           // Touch pins and programmed function

char*       dbgprint( const char* format, ... ) ;
void        tftlog ( const char *str ) ;
void        chomp ( String &str ) ;
String      nvsgetstr ( const char* key ) ;
bool        nvssearch ( const char* key ) ;
esp_err_t   nvssetstr ( const char* key, String val ) ;
void        fillkeylist() ;
const char* analyzeCmd ( const char* str ) ;
const char* analyzeCmd ( const char* par, const char* val ) ;
void reservepin ( int8_t rpinnr ) ;


//to be removed later
#define TIME_DEBOUNCE 0
#define TIME_CLICK    300
#define TIME_LONG     500
#define TIME_REPEAT   500    

#define T_DEB_IDX 0
#define T_CLI_IDX 1
#define T_LON_IDX 2
#define T_REP_IDX 3

int readSysVariable(const char *n);
const char* analyzeCmdsRR(String s);
const char* analyzeCmdRR( char* reply, String param, String value);
//end to be removed later

#if defined(RETRORADIO) 
#include <map>

#if !defined(ETHERNET)
#define ETHERNET 0
#endif

#if (ETHERNET == 2) && (defined(ARDUINO_ESP32_POE) || defined(ARDUINO_ESP32_POE_ISO))
#undef ETHERNET
#define ETHERNET 1
#else 
#if defined(ETHERNET)
#undef ETHERNET
#endif
#define ETHERNET 0
#endif


#if (ETHERNET == 1)
#include <ETH.h>
#define ETHERNET_TIMEOUT        5UL // How long to wait for ethernet (at least)?
#else 
#define ETHERNET_TIMEOUT        0UL
#endif



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


//extern volatile IR_data  ir;                          // IR data received



extern String getStringPart(String& value, char delim = ';');
extern bool getPairInt16(String& value, int16_t& x1, int16_t& x2,bool duplicate = false, char delim = ',');
extern void executeCmds(String commands);//, String value = "");
extern void doInput(String id, String value);
//extern void chomp_nvs(String & str, const char *substitute = NULL);
extern String ramgetstr ( const char* key );
extern void ramsetstr ( const char* key, String val );

extern void doCalcIfWhile(String argument, const char* type, String value);


/*
struct cmp_str
{
   bool operator()(char const *a, char const *b) const
   {
      return strcmp(a, b) < 0;
   }
};
*/

class RetroRadioInputReader {
  public:
    virtual int16_t read() = 0;
    virtual void mode(int mod) = 0;
    virtual String info() {return String("");};
  protected:
    int _mode;
};

enum LastInputType  { NONE, NEAREST, HIT };

class RetroRadioInput {
  public:
    ~RetroRadioInput();
    const char* getId();
    void setId(const char* id);
    char** getEvent(String type);
    String getEventCommands(String type, char* srcInfo = NULL);
    void setEvent(String type, const char* name);
    void setEvent(String type, String name);
    int read(bool doStart);
    void setParameters(String parameters);
    static RetroRadioInput* get(const char *id);
    static void checkAll();
    static void showAll(const char *p);
    void setReader(String value);
    void showInfo(bool hint);
  protected:
    RetroRadioInput(const char* id);
    void runClick( bool pressed = false );
    void doClickEvent( const char* type, int param = 0);
    bool hasChangeEvent();
    void executeCmdsWithSubstitute(String commands, String substitute);
    void setTiming(const char* timeName, int32_t ivalue);

    virtual void setParameter(String param, String value, int32_t ivalue);
    void setValueMap(String value, bool extend = false);
//    void clearValueMap();
    int16_t physRead();
    std::vector<int16_t> _valueMap;
//    bool _valid;
    int16_t _maximum;
  private:
    uint32_t _lastReadTime, _show, _lastShowTime;// _debounceTime;
    int16_t _lastRead, _lastUndebounceRead, _delta, _step, _fastStep, _mode;
    LastInputType _lastInputType;
//    bool _calibrate;
    char *_id;
    char *_onChangeEvent, *_on0Event, *_onNot0Event;
    char *_onClickEvent[6];
    uint8_t _clickEvents, _clickState;
    uint32_t _clickStateTime;
    int _clickLongCtr;
    static std::vector<RetroRadioInput *> _inputList;
    RetroRadioInputReader *_reader;
//    std::map<const char*, RetroRadioInput *, cmp_str> _listOfInputs;
    uint32_t _timing[4];
};


/*
class RetroRadioInputADC:public RetroRadioInput {
  public:
    RetroRadioInputADC(const char* id);
  protected:
    int16_t physRead();
  private:
    int8_t _pin;  
};

class RetroRadioInputTouch:public RetroRadioInput {
  public:
    RetroRadioInputTouch(const char* id);
  protected:
  protected:
    void setParameter(String param, String value, int32_t ivalue);
    int16_t physRead();
  private:
    touch_pad_t _pin;  
    bool _auto;
};
*/
#endif  // RETRORADIO
#endif
#endif