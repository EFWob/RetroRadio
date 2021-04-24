#ifndef RETRORADIOEXTENSION_H__
#define RETRORADIOEXTENSION_H__
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

//extern void retroRadioInit();
extern void setupRR(uint8_t setupLevel);
extern void loopRR();
extern bool analyzeCmdRR(char* reply, String& param, String& value);
extern void analyzeCmdDoneRR (char* reply, String& argument, String& value );
extern const char* analyzeCmdsRR ( String commands );
//to be removed later
int readSysVariable(const char *n);
//end to be removed later

#if defined(RETRORADIO) 
#include <map>

#if !defined(ETHERNET)
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


extern volatile IR_data  ir;                          // IR data received



extern String getStringPart(String& value, char delim = ';');
extern bool getPairInt16(String& value, int16_t& x1, int16_t& x2,bool duplicate = false, char delim = ',');
extern void executeCmds(String commands);//, String value = "");
extern void doInput(String id, String value);
extern void chomp_nvs(String & str, const char *substitute = NULL);
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
    void setEvent(String type, const char* name);
    int read(bool doStart);
    void setParameters(String parameters);
    static RetroRadioInput* get(const char *id);
    static void checkAll();
    static void showAll(const char *p);
    void setReader(String value);
    void showInfo(bool hint);
  protected:
    RetroRadioInput(const char* id);
    bool hasChangeEvent();
    void executeCmdsWithSubstitute(String commands, String substitute);


    virtual void setParameter(String param, String value, int32_t ivalue);
    void setValueMap(String value, bool extend = false);
//    void clearValueMap();
    int16_t physRead();
    std::vector<int16_t> _valueMap;
//    bool _valid;
    int16_t _maximum;
  private:
    uint32_t _lastReadTime, _show, _lastShowTime, _debounceTime;
    int16_t _lastRead, _lastUndebounceRead, _delta, _step, _fastStep, _mode;
    LastInputType _lastInputType;
//    bool _calibrate;
    char *_id;
    char *_onChangeEvent, *_on0Event, *_onNot0Event;
    static std::vector<RetroRadioInput *> _inputList;
    RetroRadioInputReader *_reader;
//    std::map<const char*, RetroRadioInput *, cmp_str> _listOfInputs;
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
