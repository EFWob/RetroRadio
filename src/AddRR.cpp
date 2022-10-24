#include "AddRR.h"
#include <Base64.h>
#include <esp_now.h>
#include <nvs.h>
#if defined(RETRORADIO)
#define sv DRAM_ATTR static volatile
#include <freertos/task.h>
#include <nvs_flash.h>
#include <nvs.h>
#include "esp_system.h"
#include "esp_spi_flash.h"
#include <map>
#include <queue>
#include <WiFi.h>
#include <genre_html.h>
#include <genres.h>
#include <ArduinoJson.h>
#include <freertos/task.h>


#if !defined(WRAPPER)
#define TIME_DEBOUNCE 50
#define TIME_CLICK    300
#define TIME_LONG     500
#define TIME_REPEAT   500    

#define T_DEB_IDX 0
#define T_CLI_IDX 1
#define T_LON_IDX 2
#define T_REP_IDX 3

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

class RetroRadioInputReader {
  public:
    virtual ~RetroRadioInputReader() {deleteContent();};
    virtual int32_t read(int8_t& forceMode)  { (void)forceMode;return 0;};
    virtual void mode(int mod)  {_mode = 0;} ;
    virtual String info() {return String("");};
    virtual void start() {_started = true;};
    virtual void stop() {_started = false;};
    virtual void specialInfo() {};
    bool started() {return _started;};
    char type() {return _type;};
    bool isVolatile() {return _volatile;};
  protected:
    virtual void deleteContent() {};
    int _mode;
    bool _started = false;
    bool _volatile = false;
    char _type = 0;
};

void substitute ( String &str, const char* substitute );
char* doprint ( const char* format, ... );

enum LastInputType  { NONE, NEAREST, HIT };

class RetroRadioInput {
  friend class RetroRadioInputReaderEvent;
  public:
    ~RetroRadioInput();
    const char* getId();
    void setId(const char* id);
    char** getEvent(String type);
    String getEventCommands(String type, char* srcInfo = NULL);
    void setEvent(String type, const char* name);
    void setEvent(String type, String name);
    int32_t read(bool doStart);
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
    int32_t physRead(int8_t& forceMode);
    std::vector<int32_t> _valueMap;
//    bool _valid;
    int32_t _maximum;
  protected:
    uint32_t _lastReadTime, _show, _lastShowTime;// _debounceTime;
    int32_t _lastRead, _lastUndebounceRead, _delta, _step, _fastStep, _mode;
    LastInputType _lastInputType;
//    bool _calibrate;
    char *_id;
    char *_onChangeEvent, *_on0Event, *_onNot0Event;
    char *_onClickEvent[6];
    uint8_t _clickEvents, _clickState;
    uint32_t _clickStateTime;
    int _clickLongCtr;
    static std::vector<RetroRadioInput *> _inputList;
    RetroRadioInputReader *_reader = NULL;
//    std::map<const char*, RetroRadioInput *, cmp_str> _listOfInputs;
    uint32_t _timing[4];
};

class RetroRadioInputReaderEvent : public RetroRadioInputReader {
  public:
      RetroRadioInputReaderEvent(RetroRadioInput* input, char type): RetroRadioInputReader(), _input(input)
        {_type = type;};
      void event(const char* data) {
        String cmd = _input->getEventCommands("change");
        if (_input->_show > 0)
          doprint("Event on EventInputReader. Started=%d, ChangeEvent=%s, data=%s", _started, 
            cmd.c_str(), data);
        if (_started)
          if (_input->_onChangeEvent) {
            substitute(cmd, data);
            if (_input->_show > 0)
              doprint("Resulting execution is: %s", cmd.c_str());
            //analyzeCmdsRR(cmd);
            cmd_backlog.push_back(cmd);
          }
      };
    protected:
      RetroRadioInput* _input;
};

class RetroRadioInputReaderMqtt : public RetroRadioInputReaderEvent {
  public:
    RetroRadioInputReaderMqtt(RetroRadioInput* input, const char *topic, bool isLocal = true) : RetroRadioInputReaderEvent(input, 'm') {
      String mqttprefix = nvsgetstr("mqttprefix");
      if (NULL == topic)
        topic = "";
      if (topic[0] == '/')
      {
        isLocal = false;
        topic++;
      }
      int tlen = strlen(topic)  + (isLocal?(mqttprefix.length() + 2):1);
      _topic =  (char *)malloc(tlen);
      if (_topic)
      {
        if (isLocal)
        {
          if (mqttprefix.length() > 0)
          {
            strcpy(_topic, mqttprefix.c_str());
            if (strlen(topic))
              strcat(_topic, "/");
          }
          else
            _topic[0] = 0;
          strcat(_topic, topic);
        }  
        else
          strcpy(_topic, topic);
        _mqttReaderList.push_back(this);
      }
    };

//ini_block.mqttprefix.length()

    virtual ~RetroRadioInputReaderMqtt() {
      std::vector<RetroRadioInputReaderMqtt *>::iterator it =  _mqttReaderList.begin();
      dbgprint("Erasing MQTT Input Reader..., topic: %s", (_topic == NULL?"<null>????":_topic));
      while (it != _mqttReaderList.end()) {
        if (*it == this)
        {
          dbgprint("Found in List, erase list...");
          _mqttReaderList.erase(it);
          it = _mqttReaderList.end();
        }
        else
          it++;
      }
      stop();
      if (_topic)
        free(_topic);
    };
    static std::vector<RetroRadioInputReaderMqtt *> _mqttReaderList;
    void specialInfo() {
      char buf[100];
      String cmnds = _input->getEventCommands("change", buf);
      if (cmnds.length() > 0) 
        {
          doprint(" * onchange-event: \"%s\" %s", cmnds.c_str(), buf);
        }
      else
        doprint(" * onchange-event is not defined!!");
    };

    void start() {
      RetroRadioInputReaderEvent::start();
      if (mqtt_on)
      {
        /*
        char s[ini_block.mqttprefix.length() + strlen(_topic) + 2];
        strcpy(s, ini_block.mqttprefix.c_str());
        if (strlen(_topic) > 0)
        {
          strcat(s, "/");
          strcat(s, _topic);
        }
        */
        if (_topic)
          mqttclient.subscribe ( _topic );
      }
    };

    void stop() {
      RetroRadioInputReaderEvent::stop();
      if (mqtt_on)
      {
        /*
        char s[ini_block.mqttprefix.length() + strlen(_topic) + 2];
        strcpy(s, ini_block.mqttprefix.c_str());
        if (strlen(_topic) > 0)
        {
          strcat(s, "/");
          strcat(s, _topic);
        }
        */
        if (_topic)
          mqttclient.unsubscribe ( _topic );
      }
    };

    String info() {        
      /*
        char buf[100];
        String ret;
        char s[ini_block.mqttprefix.length() + strlen(_topic) + 2];
        strcpy(s, ini_block.mqttprefix.c_str());
        if (strlen(_topic) > 0)
        {
          strcat(s, "/");
          strcat(s, _topic);
        }
      */
      return String("Waiting for MQTT-Messages on Topic: ") + String(_topic);
 //     ret = ret + "  *\r\n";
      //return ret;
    }

    static void begin() {
      if (mqtt_on)
        for(int i = 0; i < _mqttReaderList.size();i++)
          if (_mqttReaderList[i]->started())
            _mqttReaderList[i]->start();
    };

    static void fire(const char *topic, const char *payload) {
      /*
      int len = ini_block.mqttprefix.length();
      if (strlen(topic) < len)
        return;
      if (strlen(topic) == len)
        topic = "";
      else
        topic = topic + len + 1;
      */
      for(int i = 0; i < _mqttReaderList.size();i++)
      {
        dbgprint("FireCheck[%d]: %s==%s?->payload(%s)", i, topic, _mqttReaderList[i]->_topic, payload);
        if (_mqttReaderList[i]->_started) 
        {
          if (0 == strcmp(topic, _mqttReaderList[i]->_topic))
          {
            //dbgprint("This is a match-->fire...");  
            _mqttReaderList[i]->event(payload);
            break;
          }
        }
      }
    };

  private:
    char * _topic = NULL; 
};

std::vector<RetroRadioInputReaderMqtt *> RetroRadioInputReaderMqtt::_mqttReaderList;

void mqttInputBegin() {
  RetroRadioInputReaderMqtt::begin();
}

String doGenre ( String param, String value );
void doGenreConfig ( String param, String value );
String doGpreset ( String value );
String doFavorite ( String param, String value );
void setupReadFavorites ( void );
extern int8_t playingstat;

std::vector<int16_t> channelList;     // The channels defined currently (by command "channels")
int16_t currentChannel = 0;           // The channel that has been set by command "channel" (0 if not set, 1..numChannels else)
int16_t numChannels = 0;              // Number of channels defined in current channellist
bool setupDone = false;               // All setup for RetroRadio finished?
int16_t currentFavorite = 0;              // currently playing favorite
std::vector<char*> espnowBacklog;     // Backlog of command(s) received by espnow
bool muteBefore = false;              // how was mute before announceMode started?
uint8_t volumeBefore = 0;             // how was volume set before announceMode started?
uint32_t announceStart = 0;           // when has announceMode started?
int connectDelayBefore = 0;           // what was connectDelay before announceMode started?
extern int connectDelay;
static char cmdReply[8000] ;             // Reply to client, will be returned

String readUint8(void *);                // Takes a void pointer and returns the content, assumes ptr to uint8_t
String readInt8(void *);                 // Takes a void pointer and returns the content, assumes ptr to int8_t
String readInt16(void *);                // Takes a void pointer and returns the content, assumes ptr to int16_t
String readBool(void *);                 // Takes a void pointer and returns the content, assumes ptr to bool
String readString(void *);               // Takes a void pointer and returns the content, assumes ptr to String
String readCharStr(void *);              // Takes a void pointer and returns the content, assumes ptr to char* 
String readVolume(void *);                // Returns current volume setting from VS1053 (pointer is ignored)
String readGenrePlaylist(void *);       // Returns playable genres (with at least one station). Parameter is ignored
String readGenreName(void *);           // Returns name of current genre. Parameter is ignored.
String readMqttStatus(void *);          // Returns "1" if MQTT is connected, "0" else
String readMillis(void *);              // returns millis() as String
String readGenreCount(void *);          // returns count of known genres
String getHost(void *);                 // returns the (last successfully) connected host

String readSysVariable(const char *n);   // Read current value of system variable by given name (see table below for mapping)
const char* analyzeCmdsRR ( String commands );   // ToDo: Stringfree!!
static int genreId = 0 ;                             // ID of active genre
static uint16_t genrePresets = 0 ;                        // Nb of presets in active genre
static uint16_t genrePreset;                              // Nb of current preset in active genre (only valid if genreId > 0)
static bool gverbose = true ;                        // Verbose output on any genre related activity
//static int gchannels = 0 ;                           // channels are active on genre
uint8_t gmaintain = 0;                 // Genre-Maintenance-mode? (play is suspended)
bool resumeFlag = false;

extern WiFiClient        cmdclient ;                        // An instance of the client for commands
extern String httpheader ( String contentstype );           // get httpheader for OK response with contentstype
extern WiFiClient        wmqttclient ;                          // An instance for mqtt
extern bool mqtt_on;
extern int8_t            connecttohoststat ;              // Status of connecttohost


#define KNOWN_SYSVARIABLES   (sizeof(sysVarReferences) / sizeof(sysVarReferences[0]))



struct {                           // A table of known internal variables that can be read from outside
  const char *id;
  void *ref;
  String (*readf)(void*);
} sysVarReferences[] = {
  {"volume", &ini_block.reqvol, readUint8},
  {"volume_before", &volumeBefore, readUint8},
  {"preset", &ini_block.newpreset, readInt16},
  {"toneha", &ini_block.rtone[0], readUint8},
  {"tonehf", &ini_block.rtone[1], readUint8},
  {"tonela", &ini_block.rtone[2], readUint8},
  {"tonelf", &ini_block.rtone[3], readUint8},
  {"channel", &currentChannel, readInt16},
  {"channels", &numChannels, readInt16},
  {"volume_vs", &numChannels, readVolume},
  {"genre", &genreId, readInt16},
  {"genres", NULL, readGenreCount},
  {"gname", NULL, readGenreName},
  {"gpresets", &genrePresets, readInt16},
  {"gpreset", &genrePreset, readInt16},
  {"glist", NULL, readGenrePlaylist},
  //{"url", &currentStation, readString},
  {"url_before", &stationBefore, readString},
  {"icyname", &icyname, readString},
  {"icystreamtitle", &icystreamtitle, readString},
  {"mqtt_on", NULL, readMqttStatus},
  {"playing", &playingstat, readInt8},
  {"favorite", &currentFavorite, readInt16},
  {"mute", &muteflag, readBool},
  {"mute_before", &muteBefore, readBool},
  {"hour", &timeinfo.tm_hour, readInt16},
  {"minute", &timeinfo.tm_min, readInt16},
  {"second", &timeinfo.tm_sec, readInt16},
  {"time", &timetxt, readCharStr},
  {"uptime", NULL, readMillis},
  {"connecthost", &connecttohoststat, readInt8},
  {"url", NULL, getHost},
  {"announce", &announceMode, readUint8}
};


const char* analyzeCmdsRR ( String s, bool& returnFlag ) ;

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
int eth_phy_addr  =     ETH_PHY_ADDR ;    // Ethernet Phy Address setting
int eth_phy_power =   12; // ETH_PHY_POWER ;   // Ethernet Power setting
int eth_phy_mdc   =      ETH_PHY_MDC ;     // Ethernet MDC setting
int eth_phy_mdio  =   ETH_PHY_MDIO ;    // Ethernet Phy MDIO setting
int eth_phy_type  =     ETH_PHY_TYPE ;    // Ethernet Phy Type setting
int eth_clk_mode  =   3;//  ETH_CLK_MODE ;    // Ethernet clock mode setting
uint32_t eth_timeout =  ETHERNET_TIMEOUT ;    // Ethernet timeout (in seconds)

//TODO Read ethernet prefs from NVS!
#else 
#define ETHERNET_TIMEOUT        0UL
#endif

//**************************************************************************************************
//                                      N V S D E L K E Y                                          *
//**************************************************************************************************
// Deleta a keyname in nvs.                                                                        *
//**************************************************************************************************
void nvsdelkey ( const char* k)
{

  if ( nvssearch ( k ) )                                   // Key in nvs?
  {
    nvs_erase_key ( nvshandle, k ) ;                        // Remove key
  }
}




//**************************************************************************************************
//                                          D O P R I N T                                          *
//**************************************************************************************************
// Copy of dbgprint. Only difference: will always print but only prefix "D: ", if DEBGUG==1        *
//**************************************************************************************************
char* doprint ( const char* format, ... )
{
  static char sbuf[2 * DEBUG_BUFFER_SIZE] ;                // For debug lines
  va_list varArgs ;                                    // For variable number of params
  va_start ( varArgs, format ) ;                       // Prepare parameters
  vsnprintf ( sbuf, sizeof(sbuf), format, varArgs ) ;  // Format the message
  sbuf[2 * DEBUG_BUFFER_SIZE - 1] = 0;
  va_end ( varArgs ) ;                                 // End of using parameters
#if !defined(NOSERIAL)
  if ( DEBUG )                                         // DEBUG on?
  {
    Serial.print ( "D: " ) ;                           // Yes, print prefix
  }
  Serial.println ( sbuf ) ;                            // always print the info
  Serial.flush();
 #endif
  if (DEBUGMQTT)
    if (mqtt_on)
    {
      char *s = (char *)malloc(10 + strlen(sbuf));
      if (s)
      {
        strcpy(s, "debug");
        strcpy(s + 6, sbuf);
        mqttpub_backlog.push_back(s);
      }
    }
  return sbuf ;                                        // Return stored string
}



// Read current value of system variable by given name (see table sysvarreferences)
// Returns 0 if variable is not defined

String readSysVariable(const char *n) {
  for (int i = 0; i < KNOWN_SYSVARIABLES; i++)
    if (0 == strcmp(sysVarReferences[i].id, n))
      return sysVarReferences[i].readf(sysVarReferences[i].ref);
  return String("");
}

String readUint8(void *ref) {
  return String( (*((uint8_t*)ref)));
}

String readInt8(void *ref) {
  return String( (*((int8_t*)ref)));
}

String readInt16(void *ref) {
  dbgprint("Inblock.newpreset = %d", ini_block.newpreset);
  return String( (*((int16_t*)ref)));
}

String readBool(void *ref) {
  return String( (*((bool*)ref)));
}

String readString(void *ref) {
  return  (*((String*)ref));
}

String readCharStr(void *ref) {
  return  (String((char*)ref));
}


String readMillis(void *)
{
  return String(millis());
}

String readGenreCount(void *) 
{
  return String(genres.size());
} 


String readVolume(void* ref) {
  if (vs1053player)
    return String(vs1053player->getVolume (  )) ;
  else
    return String(ini_block.reqvol);
}

String readMqttStatus(void *) {
  if (mqtt_on)
    if (wmqttclient.connected())
      return "1";
  return "0";
}

String getHost(void *) {
  if (1 == connecttohoststat)
    return host;
  return lasthost;
}

String doSysList(const char* p) {
  String ret = "{\"syslist\"=[";
  bool first = true;
  doprint("Known system variables (%d entries)", KNOWN_SYSVARIABLES);
  for (int i = 0; i < KNOWN_SYSVARIABLES; i++)
  { 
    bool match = p == NULL;
    if (!match)
      match = (NULL != strstr(sysVarReferences[i].id, p));
    if (match)  {
      String s = sysVarReferences[i].readf(sysVarReferences[i].ref);
      doprint("%2d: '%s' = %s", i, sysVarReferences[i].id, s.c_str());
      if (first)
        first = false;
      else
        ret = ret + ",";
      ret = ret + "{\"id\": \"" + String(sysVarReferences[i].id) + "\", \"val\":\"" + s + "\"}"; 
    }
  } 
  return ret + "]}";
}

bool isAnnouncemode() {
  return 0 != (announceMode & ~ANNOUNCEMODE_RUNDOWN);
  //return ((announceMode !=0 ) && (announceMode <= 2));
}

void announceRecoveryFull() {
  char s[stationBefore.length() + 20];
  announceMode = 0;
  String info = stationBefore ;
  dbgprint("Running defaultAlertStop, volume=%d, mute=%d, url=%s", volumeBefore, muteBefore, stationBefore.c_str());
  muteflag = muteBefore ;
  ini_block.reqvol = volumeBefore ;
  connectDelay = connectDelayBefore;
  sprintf(s, "station=%s", stationBefore.c_str());
  analyzeCmd(s);
  knownstationname = knownstationnamebefore;
  icystreamtitle = "";
}


void announceRecoveryPrio3()
{
  int idx;
  if (resumeFlag)
  {
    announceRecoveryFull();
  }
  else
  {
    //host = currentStation;
    idx = host.indexOf('#');
    if (idx>0)
    {
      host = host.substring(0, idx);
      host.trim();
    }
    hostreq = true;  
    announceMode = ANNOUNCEMODE_PRIO3;
  }
}


void announceRecoveryPrio1()
{
const char *s = "::alertdone";
String commands = ramsearch(s)?ramgetstr(s) : nvsgetstr(s);
  if ((commands.length() > 0) && !resumeFlag)
  {
    announceMode = ANNOUNCEMODE_RUNDOWN;
    dbgprint("Now execute commands: %s", commands.c_str());
    analyzeCmd(commands.c_str());
    if (!isAnnouncemode()) {
      announceMode = 0;        
    }
  }
  else
  {
    announceRecoveryFull();
  }
}


void announceRecoveryPrio0() {
String info = stationBefore;
char s[stationBefore.length() + 20];
    announceMode = 0;
    sprintf(s, "station=%s", stationBefore.c_str());
    analyzeCmd(s);
    knownstationname = knownstationnamebefore;
    //setLastStation(info);        
}

bool setAnnouncemode(uint8_t mode) {
bool ret = false;
  if ((0 != mode) && (0 != (announceMode & ANNOUNCEMODE_NOCANCEL)))
    return false;
  if (0 == mode)
  {
    ret = true;
    if (0 != (announceMode && ~ANNOUNCEMODE_RUNDOWN))
    {
      uint8_t recoverStrategy = announceMode & ANNOUNCEMODE_PRIOALL;
      if (recoverStrategy & ANNOUNCEMODE_PRIO3)
      {
          announceRecoveryPrio3();
      }
      else if (recoverStrategy & ANNOUNCEMODE_PRIO2)
      {
          announceRecoveryPrio0();
      }
      else if (recoverStrategy & ANNOUNCEMODE_PRIO1)
      {
          announceRecoveryPrio1();
      }
      else if (recoverStrategy & ANNOUNCEMODE_PRIO0)
      {
          announceRecoveryPrio0();
      }
    }
  }
  else
  {
    dbgprint("Check if %d>=%d", mode, announceMode);
    if ((mode >= announceMode) && (0 == (announceMode & ANNOUNCEMODE_RUNDOWN)))
    {
      uint8_t newPrio = (ANNOUNCEMODE_PRIOALL + 1);
      do 
      {
        newPrio = newPrio >> 1;
      } while (newPrio && (0 == (mode & newPrio)));
      if (0 == newPrio)
        return false;
      ret = true;
      dbgprint("OK, An new announcement with at least same priority...");
      if (0 == announceMode)
      {
        stationBefore = lasthost;//currentStation;
        muteBefore = muteflag ;
        volumeBefore = ini_block.reqvol ;
        muteflag = false ;
      }
      connectDelay = 0 ;
      streamDelay = 500;
      mode = newPrio | (mode & ~ANNOUNCEMODE_PRIOALL);
      if (((mode & 2) == 2) && ((announceMode & 2) != 2))
      {
        const char *s = "::alertstart";
        //String commands = 
        connectcmds = ramsearch(s)?ramgetstr(s) : nvsgetstr(s);
        /*
        if (commands.length() > 0)
          {
            //dbgprint("Now execute commands: %s", commands.c_str());
            analyzeCmd(commands.c_str());
          }
        else
          dbgprint("Nothing to do for '%s'!", s);
        */
      }
      announceMode = mode;
    }
  }   
  if (ret)
  {
    resumeFlag = false;
    announceStart = millis();
    if (0 == announceStart)
      announceStart = 1;
  }
  return ret;
}

void getAnnounceInfo(String& url, String& info, uint8_t id) {
  int idx = url.indexOf(' ');
  if (idx > 0)
  {
    info = url.substring(idx + 1);
    info.trim();
    url = url.substring(0, idx);
  }
  else
  {
    const char *defaultinfo = "Alarm!";
    const char *searchkey = NULL;
    switch(id)
    {
      case 1:
        searchkey = "@$announceinfo";
        defaultinfo = "Announcement";
        break;
      case 2:
        searchkey = "@$alertinfo";
        defaultinfo = "Alert!";
        break;
      default:
        break;
    }
    if (searchkey) {
      info = String(searchkey);
      chomp_nvs(info);
    }
    if (info.length() == 0)
      info = String(defaultinfo);
  }
  if (knownstationname.length() > 0)
    knownstationnamebefore = knownstationname;
  else
    knownstationnamebefore = icyname;
  knownstationname = info;
  icystreamtitle = info;
}


/**********************************************************************************************************/
/*                             R E A D A R G U M E N T S                                                  */
/**********************************************************************************************************/
/* Split arguments contained in parameter value and separate them into std::vector args                   */
/* A single argument is either                                                                            */
/*     - a word (separated by space)                                                                      */
/*     - a sequence of words surrounded by double quotes ("this is a sequence")                           */
/*        - if double quotes are needed within a sequence, they must be doubled (c-style), i. e.          */
/*          "this is a sequence containing a ""quoted"" word"                                             */
/*     - surrounding quotation marks will be deleted before storing the argument into the vector          */
/*     - Both forms can be mixed, but a whitespace is also requested after/before quotation marks:        */
/*          Parameter1 "This is param2" another param                                                     */
/*        will result in 4 arguments:                                                                     */
/*            1: Parameter1                                                                               */
/*            2: This is param2                                                                           */
/*            3: another                                                                                  */    
/*            4: param                                                                                    */
/**********************************************************************************************************/

void readArguments(std::vector<String>& args, String value) {
  args.clear();
  value.trim();
  char *cpy = strdup(value.c_str());
  if (cpy)
  {
    char *start = cpy;
    do 
    {
      while ((*start > 0) && (*start < ' '))
        start++;
      if (*start)
      {
        char *nextStart;
        if (*start == '"')
        {
          bool done = false;
          start++;
          nextStart = start;
          do {
            nextStart = strchr(nextStart, '"');
            if (!nextStart)
              done = true;
            else {
              strcpy(nextStart, nextStart + 1);
              done = ('"' != *nextStart);
              if (!done)
                nextStart++;
            }
          } while (!done);
        }
        else
        {
          nextStart = start + 1;
          while (*nextStart > ' ')
            nextStart++;
        }
        if (nextStart)
          if (*nextStart)
          {
            *nextStart = 0;
            nextStart++;
          }
        args.push_back(String(start));
        if (nextStart)
          start = nextStart;
        else
          start = start + strlen(start);
      }
    } while (*start);
    free(cpy);
  }
}

void doArguments(String value)
{
  std::vector<String> args;
  readArguments(args,value);
  dbgprint("Found %d arguments!", args.size());
  for (int i= 0;i < args.size();i++)
    dbgprint("%2d: %s", i, args[i].c_str());
}

// Obsolete?
void readDataList(std::vector<int16_t>& v, String value) {
  char *s = strdup(value.c_str());
  if (s) {
    char *first = s;
    while (first) {
      char * next = strchr(first, ',');
      if (NULL != next) {
        *next = 0;
        next++;
      }
      v.push_back(atoi(first));
      first = next;
    }

    free(s);
  }
}


void substitute ( String &str, const char* substitute ) {
  if (substitute) {
    int idx = str.indexOf('?');
    if (idx >= 0) {
      idx = 0;
      int nesting = 0;
      int subLen = strlen(substitute);
      while (idx < str.length()) {
        if (str.c_str()[idx] == '{')
          nesting++;
        else if (str.c_str()[idx] == '}') {
          if (nesting > 0)
            nesting--;
        } else if (str.c_str()[idx] == '?')
          if (0 == nesting) {
            str = str.substring(0, idx) + String(substitute) + str.substring(idx + 1);
            idx = idx + subLen - 1;
          }
        idx++;
      }
    }
    /*
    while(idx >= 0) {
      str = str.substring(0, idx) + String(substitute) + str.substring(idx + 1);
      idx = str.indexOf('?');
    }
    */
  }
  
}

//**************************************************************************************************
//                                    C H O M P _ N V S                                            *
//**************************************************************************************************
// Do some filtering on the inputstring:                                                           *
//  - do 'normal' chomp first, return if resulting string does not start with @                    *
//  - if resulting string starts with '@', nvs is looked if a key with the name following @ is     *
//    found in nvs. If so, that string (chomped) is returned or empty if key is not found          *
//**************************************************************************************************
void chomp_nvs ( String &str )
{
  //Serial.printf("Chomp NVS with %s\r\n", str.c_str());Serial.flush();
  chomp ( str ) ;                                     // Normal chomp first
  char first = str.c_str()[0];
  if (first == '@' )                         // Reference to RAM or NVS-Key?
    {
    if ( ramsearch (str.c_str() + 1 ) ) 
      { 
      //Serial.printf("NVS Search success for key: %s\r\n", str.c_str() + 1);
      str = ramgetstr ( str.c_str() + 1) ;
      chomp ( str ) ;
      } 

    else if ( nvssearch (str.c_str() + 1 ) ) 
      { 
      //Serial.printf("NVS Search success for key: %s, result =", str.c_str() + 1);
      str = nvsgetstr ( str.c_str() + 1) ;
      chomp ( str ) ;
      //Serial.println(str);
      }
      else
        str = "";   
    } 
  else if (first == '~' )                         // Reference to System Variable?
    {
      str = readSysVariable(str.c_str() + 1);
    }
   else if (first == '.' )                         // Reference to RAM?
    {
      str = ramgetstr ( str.c_str() + 1) ;
      chomp ( str ) ;
    }
   else if (first == '&' )                         // Reference to NVS?
    {
      str = nvsgetstr ( str.c_str() + 1) ;
      chomp ( str ) ;
    }
}





//**************************************************************************************************
//                                S P L I T                                                        *
//**************************************************************************************************
// Splits the input String value at the delimiter identified by parameter delim.                   *
//  - Returns left part of the string, chomped                                                     *
//  - Input String will contain the remaining part after delimiter (not altered)                   *
//  - will return the input string if delimiter is not found
//**************************************************************************************************
String split(String& value, char delim) {
  int idx = value.indexOf(delim);
  String ret = value;
  if (idx < 0) {
    value = "";
  } else {
    ret = ret.substring(0, idx);
    value = value.substring(idx + 1);
  }
  chomp(ret);
  return ret;
}

String split(String& value, char delim, bool protect)
{ 
  if (!protect)
    return split(value, delim);
String ret = value;
int nesting = 0;
char nestingChar = 0;
char nestingEndChar = 0;
const char *s; 
const char *s1 = s = value.c_str();
  while (*s) 
  {
    if (*s == delim)
      if (0 == nesting)
        break;            
    if ( *s == nestingChar)
      nesting++;
    else if ( *s == nestingEndChar ) 
    {
      if ( 0 == --nesting )
        nestingEndChar = nestingChar = 0;
    }
    else if ( 0 == nesting) 
      if ( *s == '(' ) 
      {
        nestingChar = '(';
        nestingEndChar = ')';
        nesting = 1;
      }
      if ( *s == '{' ) 
      {
        nestingChar = '{';
        nestingEndChar = '}';
        nesting = 1;
      }
    s++;
  }
  int idx = s - s1;
  ret = ret.substring(0, idx);
  value = value.substring(idx + 1);
  chomp(ret);
  //ret.toLowerCase();
  return ret;  
}


//**************************************************************************************************
//                          G E T P A I R I N T 1 6                                                *
//**************************************************************************************************
// Extracts a pair of int16 values from String                                                     *
//  - Returns true, if two values are found (will be stored in parameters x1, x2)                  *
//  - Also true, if only one is found but parameter duplicate is set to true (x2 will set to x1).  *
//  - Remaining characters are ignored.                                                            *
//**************************************************************************************************


bool getPairInt16(String& value, int16_t& x1, int16_t& x2, bool duplicate, char delim) {
  bool ret = false;
  chomp(value);
  if ((value.c_str()[0] == '-') || isdigit(value.c_str()[0])) 
  {
    int idx = value.indexOf(delim);
    x1 = atoi(value.c_str());
    ret = (idx > 0);
    if ( ret ) 
    {
      x2 = atoi(value.substring(idx + 1).c_str());
    } 
    else 
    {
      ret = duplicate;
      if ( ret )
        x2 = x1;
    }
  }
  return ret;
}

String extractgroup(String& inValue) {
  String ret;
  inValue.trim();
  if (inValue.c_str()[0] != '(') {
    ret = inValue;
    inValue = "";
  } else {
    const char *p = inValue.c_str() + 1;
    int nesting = 1;
    while (*p && nesting) {
      if (*p == '(')
        nesting++;
      else if (*p == ')')
        nesting--;
      if (nesting)
        p++;
    }
    ret = inValue.substring(0, p - inValue.c_str());
    if (*p)
      inValue = String(p + 1);
    else
      inValue = "";
  }
  return ret;
}

//**************************************************************************************************
//                          P A R S E G R O U P                                                    *
//**************************************************************************************************
// Extracts the left and the right part of a group                                                 *
//  - A group is considered to be enclosed by round paranthesis ()                                 *
//  - Groups can be nested.                                                                        *
//  - The opening paranthesis is optional but expected to be the first char of input value         *
//  - If no closing paranthesis is found, full input value is considered to be the group           *
//  - Remaining characters are ignored.                                                            *
//  - String inValue is set to the contents of remainder after the group                           *
//  - Group content (if any) is then split in left and right part by the strings contained in the  *
//    delimters arry. Last entry in delimiters arry must be NULL to denote end of delimiters       *
//  - If no delimiter is found, right part will be empty String("")                                *
//  - returns the index of used delimiter within array of delimters or < 0 if not found            *
//  - If delimiters==NULL, left will be set to full group (and -1 is returned)                     *
//  - If -1 is returned, right will be empty String("")                                            *
//  - All altered Strings (inValue, left, right) will be chomped upon return                       *
//  - If extendNvs (defaults to false) is true, left and right will be assumed to be reference to  *
//    nvs-key if preceeded by '@'                                                                  *
//**************************************************************************************************
int parseGroup(String & inValue, String& left, String& right, const char** delimiters, bool extendNvs,
        bool extendNvsLeft, bool extendNvsRight) {
  int idx = -1, i;
  bool found;
  inValue.trim();
  int nesting = 0;
  bool commandGroup = (inValue.c_str()[0] == '{');
  char groupStart = '(';
  char groupEnd = ')';
  if ((inValue.c_str()[0] == '(') || commandGroup) {
    if (commandGroup) {
      groupStart = '{';
      groupEnd = '}';
    }
    inValue = inValue.substring(1);
    chomp(inValue);
    nesting = 1;
  }
  //    else return -1;
  const char *p = inValue.c_str();
  idx = -1;
  while (*p && (idx == -1)) {
    if (*p == groupStart)
      nesting++;
    else if (*p == groupEnd) {
      if (nesting <= 1)
        idx = p - inValue.c_str();
      else
        nesting--;
    }
    p++;
  }
  //  idx = inValue.indexOf(')');
  if (idx == -1) {
    left = inValue;
    inValue = "";
  } else {
    left = inValue.substring(0, idx);
    chomp(left);
    inValue = inValue.substring(idx + 1);
    chomp(inValue);
  }
  found = false;
  i = 0;
  if (delimiters)
    while (!found && (delimiters[i] != NULL))
      if (!(found = (-1 != (idx = left.indexOf(delimiters[i])))))
        i++;
  if (found) {
    right = left.substring(idx + strlen(delimiters[i]));
    left = left.substring(0, idx);
    if (extendNvs) {
      chomp_nvs(left);
      chomp_nvs(right);
    } else {
      if (extendNvsLeft)
        chomp_nvs(left);
      else
        chomp(left);
      if (extendNvsRight)
        chomp_nvs(right);
      else
        chomp(right);
    }
    return i;
  } else {
    right = "";
    if (extendNvs)
      chomp_nvs(left);
    return -1;
  }
}



int32_t doCalculation(String& value, bool show, const char* ramKey = NULL) {
  const char *operators[] = {"==", "!=", "<=", "><" , ">=", "<", "+", "^", "*", "/", "%", "&&", "&", "||", "|", "-", ">", "..", NULL};
  String opLeft, opRight;
  int idx = parseGroup(value, opLeft, opRight, operators, true);
  int32_t op1, op2, ret = 0;
  if (show) doprint("Calculate: (\"%s\" [%d] \"%s\")", opLeft.c_str(), idx, opRight.c_str());
  if (show) doprint("Remains: %s", value.c_str());
  if (opLeft.c_str()[0] == '(')
    op1 = doCalculation(opLeft, show);
  else
    op1 = opLeft.toInt();
  if (opRight.c_str()[0] == '(')
    op2 = doCalculation(opRight, show);
  else
    op2 = opRight.toInt();
  switch (idx) {
    case -1:
      ret = op1;
      break;
    case 0:
      ret = op1 == op2;
      break;
    case 1:
      ret = op1 != op2;
      break;
    case 5:
      ret = op1 < op2;
      break;

    case 2:
      ret = op1 <= op2;
      break;
    case 3: case 17:
      ret = random(op1, op2 + 1);
      break;
    case 4:
      ret = op1 >= op2;
      break;
    case 6:
      ret = op1 + op2;
      break;
    case 7:
      ret = op1 ^ op2;
      break;
    case 8:
      ret = op1 * op2;
      break;
    case 9:
      ret = op1 / op2;
      break;
    case 10:
      ret = op1 % op2;
      break;
    case 11:
      ret = op1 && op2;
      break;
    case 12:
      ret = op1 & op2;
      break;
    case 13:
      ret = op1 || op2;
      break;
    case 14:
      ret = op1 | op2;
      break;
    case 15:
      ret = op1 - op2;
      break;
    case 16:
      ret = op1 > op2;
      break;
  }
  if (show)
    doprint("Caclulation result=%d", ret);
  if (ramKey)
    if (strlen(ramKey)) {
      ramsetstr(ramKey, String(ret));
      if (show) doprint("Calculation result set to ram.%s=%d", ramKey, ret);
    }
  return ret;
}

/*
void doIf(String value, bool show, bool& returnFlag) {
  String ifCommand, elseCommand;
  String dummy;
  int isTrue;
  if (show) doprint("Start if=\"%s\"", value.c_str());
  isTrue = doCalculation(value, show);
  parseGroup(value, ifCommand, dummy);
  parseGroup(value, elseCommand, dummy);
  dummy = String(isTrue);
  if (show) {
    doprint("  IfCommand = \"%s\"", ifCommand.c_str());
    doprint("ElseCommand = \"%s\"", elseCommand.c_str());
    doprint("Condition is %s (%d)", isTrue ? "TRUE" : "false", isTrue); Serial.flush();
  }
  if (isTrue) {
    substitute(ifCommand, dummy.c_str());
    if (show) doprint("Running \"if(true)\" with (substituted) command \"%s\"", ifCommand.c_str());
    analyzeCmdsRR(ifCommand);
  } else {
    substitute(elseCommand, dummy.c_str());
    if (show) doprint("Running \"else(false)\" with (substituted) command \"%s\"", elseCommand.c_str());
    analyzeCmdsRR(elseCommand);
  }
}

void doCalc(String value, bool show, bool& returnFlag) {
  String calcCommand;
  String dummy;
  int result;
  if (show) doprint("Start calc=\"%s\"", value.c_str());
  result = doCalculation(value, show);
  parseGroup(value, calcCommand, dummy);
  dummy = String(result);
  if (calcCommand == "0")
    calcCommand = "";
  if (show) {
    doprint("CalcCommand is now = \"%s\"", calcCommand.c_str());
  }
  if ( calcCommand.length() > 0 ) 
  {
    substitute(calcCommand, dummy.c_str());
    if (show) doprint("Running \"calc\" with (substituteded) command \"%s\"", calcCommand.c_str());
    analyzeCmdsRR(calcCommand);
  }
}
*/

void doIf(String condition, String value, bool show, String ramKey, bool& returnFlag, bool invertedLogic) {
  if (show) doprint("Start if(%s)=\"%s\"", condition.c_str(), value.c_str());
  int32_t result = doCalculation(condition, show, ramKey.c_str());

  String ifCommand, elseCommand;
  String dummy;
  parseGroup(value, ifCommand, dummy);
  parseGroup(value, elseCommand, dummy);
  dummy = String(result);
  if (show) {
    doprint("  IfCommand = \"%s\"", ifCommand.c_str());
    doprint("ElseCommand = \"%s\"", elseCommand.c_str());
    doprint("Condition is %s (%d)", (result != 0) ? "TRUE" : "false", result); Serial.flush();
  }
  bool doIf = (result != 0);
  if (invertedLogic)
    doIf = !doIf;
  if (doIf) {
    substitute(ifCommand, dummy.c_str());
    if (show) doprint("Running \"if(true)\" with (substituted) command \"%s\"", ifCommand.c_str());
    analyzeCmdsRR ( ifCommand, returnFlag );
  } else {
    substitute(elseCommand, dummy.c_str());
    if (show) doprint("Running \"else(false)\" with (substituted) command \"%s\"", elseCommand.c_str());
    analyzeCmdsRR ( elseCommand, returnFlag );
  }
}

void doCalc(String expression, String value, bool show, String ramKey, bool& returnFlag) {
  if (show) doprint("Start calc(%s)=\"%s\"", expression.c_str(), value.c_str());
  int32_t result = doCalculation(expression, show, ramKey.c_str());

  String calcCommand;
  String dummy;

  parseGroup(value, calcCommand, dummy);
  dummy = String(result);
  if ( calcCommand == "0" )
    calcCommand = "";
  if (show) {
    doprint("CalcCommand = \"%s\"", calcCommand.c_str());
  }
  if ( calcCommand.length() > 0 )
  {
    substitute(calcCommand, dummy.c_str());
    if (show) doprint("Running \"calc\" with (substituted) command \"%s\"", calcCommand.c_str());
    analyzeCmdsRR ( calcCommand, returnFlag );
  }
}

//***********************************************************************************************************
//* searchElement                                                                                           *
//***********************************************************************************************************
//  - scan throug the comma delimited list of "input"                                                       *
//  - return the list element indicated by idx (idx with 1)                                                 *  
//  - if idx <=1                                                                                            *
//        - empty string is returned if parameter elements is NULL                                          *
//        - *elements is set to number of list elements else                                                *
//  - outputs some verbose information if parameter show is set to true                                     *
//***********************************************************************************************************
String searchElement(String input, int& idx, bool show = false, int* elements = NULL)
{
  bool justCount = ((NULL != elements) && (idx <= 0));                            // just counting the elements?
  String result = String("");
  if ((0 >= idx) && !justCount)                                                   // idx is below 1?             
    return result;
  chomp(input);                                                                   // just to make sure
  if (input.length() == 0)
    return (justCount?String("0"):result);                                                            
  if (show) 
    if (!justCount)
      doprint("Search idx(%d) in %s", idx, input.c_str());
  if (strchr(input.c_str(), ','))                                                 // more than one element in current list?
    do
    {
      String left = split(input, ',');                                    // separate one element
      if (show) 
        doprint("Searching deeper with %s (idx is still %d)", left.c_str(), idx);
      result = searchElement(left, idx, show, elements);                          // check leftmost entry (could be a derefenced list in itself)
    }
    while ((input.length() > 0) && (justCount || (idx > 0)));
  else if (NULL != strchr("@&.~", input.c_str()[0]))                               // No ',' found, but pointer to RAM/NVS
  {
    if (show) 
      doprint ("resolving (and analyzing) key %s", input.c_str()) ;
    chomp_nvs(input);                                                             // get RAM/NVS entry
    result = searchElement (input, idx, show, elements);                          // evaluate the dereferenced list element (could be "embedded" list)
  }
  else                                                                            // here one single entry left, not dereferrenced
  {
    if ((idx == 1) && !justCount)                                                 // index found?
      result = input;                                                             // set result  
    idx--;
    if (justCount)                                                                // are we just counting?
        result = ++(*elements);                                                   // increase the count
  }
  return result;
}

void doIdx(String expression, String value, bool show, String ramKey, bool& returnFlag) {
  bool hasAnyEntry,justCount;
  int idx = 0;
  int elements = 0;
  String idxCommand;
  String left, right;
  String result;
  if (show) doprint("Start idx(%s)=\"%s\"", expression.c_str(), value.c_str());
 
  
  parseGroup(value, idxCommand, right);
//  hasAnyEntry = strchr(expression.c_str(), ',') != NULL;
  left = split(expression, ',');
  hasAnyEntry = expression.length() > 0;
  //chomp_nvs(left);
//  if (!(justCount = (left == "0")))
  //idx = left.toInt();
  idx = doCalculation(left, show, "");
  justCount = idx <= 0;
  if (show) 
  {
    if (justCount)
      doprint("Counting elements in list '%s'", expression.c_str());
    else
      doprint("Searching for element %d in list '%s'", idx, expression.c_str());
  }
  if (hasAnyEntry)
    {
      if(show) 
        doprint("Search %s, idx==%d", left.c_str(), idx);
      result = searchElement(expression, idx, show, &elements);
    }
  else if (justCount)
    result = "0";
  if (show) 
  {
    if (justCount)
      doprint("Looks like the list has %s entrie(s)", result.c_str());
    else
      doprint("Result = '%s' (found: %d)", result, idx == 0);
  }
  if (ramKey.length() > 0)
    ramsetstr(ramKey.c_str(), result);

  if ( idxCommand == "0" )
    idxCommand = "";
  if (show) {
    doprint("IdxCommand = \"%s\"", idxCommand.c_str());
  }
  if ( idxCommand.length() > 0 )
  {
    substitute(idxCommand, result.c_str());
    if (show) doprint("Running \"idx\" with (substituted) command \"%s\"", idxCommand.c_str());
    analyzeCmdsRR ( idxCommand, returnFlag );
  }
}


void doLen(String expression, String value, bool show, String ramKey, bool& returnFlag) {
  String result;
  if (show) 
    doprint("Start len(%s)=\"%s\"", expression.c_str(), value.c_str());
  chomp_nvs(expression);
  result = String(expression.length());
  if (show)
    doprint("Length of expression '%s'=%s", expression.c_str(), result.c_str());
  if (ramKey.length() > 0)
    ramsetstr(ramKey.c_str(), result);
  if ( value == "0" )
    value = "";
  if (show) {
    doprint("LenCommand = \"%s\"", value.c_str());
  }
  if ( value.length() > 0 )
  {
    substitute(value, result.c_str());
    if (show) 
      doprint("Running \"len\" with (substituted) command \"%s\"", value.c_str());
    analyzeCmdsRR ( value, returnFlag );
  }
}



void doWhile(String conditionOriginal, String valueOriginal, bool show, String ramKey, bool& returnFlag, bool invertedLogic) {
  bool done = false;
  if (show) doprint("Start while(%s)=\"%s\"", conditionOriginal.c_str(), valueOriginal.c_str());
  do {
    String value = valueOriginal;
    String condition = conditionOriginal;
    int result = doCalculation(condition, show, ramKey.c_str());
    done = (0 == result);
    if (invertedLogic)
      done = !done;
    if (!done) {
      String whileCommand, dummy;
      parseGroup(value, whileCommand, dummy);
      if (show)
        doprint("while(%d)=\"%s\"", result, whileCommand.c_str());
      dummy = String(result);
      substitute(whileCommand, dummy.c_str());
      if (show) doprint("Running \"while\" with (substituted) command \"%s\"", whileCommand.c_str());
      analyzeCmdsRR ( whileCommand, returnFlag );
    }
    yield();
  } while (!done);
}

void doCalcIfWhile(String command, const char *type, String value, bool& returnFlag/*, bool invertedLogic = false*/) {
/*
  if (0 == strcmp("idx", type))
  {
    doprint("Start doCalcIfWhile");
    delay(1000);
    doprint("Parameters are: command=%s, type=%s, value=%s", command.c_str(), type, value.c_str());
    delay(1000);
  }
  */
  String ramKey, cond, dummy;
  int commandLen = strlen(type);
  dummy = command.substring(commandLen, commandLen + 3);
  dummy.toLowerCase();
  bool invertedLogic =  dummy == "not";
  const char *s = command.c_str() + commandLen + (invertedLogic?3:0);
  bool show =  tolower(*s)  == 'v';
  if (show)
    s++;
  //show = true;
  while ((*s != 0) && (*s <= ' '))
    s++;
  char *condStart = strchr(s, '(');
  if (!condStart) {
    if (show)
      doprint("No condition/expression found for %s=()", type);
    return;
  } else {
    dummy = String(condStart);
    if (show)
      doprint("Start extracting condition from %s", dummy.c_str());
    parseGroup(dummy, cond, dummy);
    if (show)
      doprint("Extracted condition is: %s", cond.c_str());
  }
  if (*s == '.') {
    ramKey = String(s + 1);
    ramKey = ramKey.substring(0, ramKey.indexOf('('));
    //s = s + ramKey.length();
    ramKey.trim();
  }
  if (show)
    doprint("ParseResult: %s(%s)=%s; Ramkey=%s", type, cond.c_str(), value.c_str(), ramKey.c_str());
  switch (*(type + 1)) {
    case 'h':
      doWhile(cond, value, show, ramKey, returnFlag, invertedLogic);
      break;
    case 'a':
      doCalc(cond, value, show, ramKey, returnFlag);
      break;
    case 'f':
      doIf(cond, value, show, ramKey, returnFlag, invertedLogic);
      break;
    case 'd':
      doIdx(cond, value, show, ramKey, returnFlag);
      break;
    case 'e':
      doLen(cond, value, show, ramKey, returnFlag);
      break;

  }
}

//**************************************************************************************************
// Class: RetroRadioInput                                                                          *
//**************************************************************************************************
// Generic class for Input (from GPIOs normally)                                                   *
//  - Needs subclasses to implement different input types (Digital, Analog, Touch...)              *
//  - Subclass must implement int physRead(); function to return current value of input             *
//  - Class RetroRadioInput provides common functionality like debouncing and mapping the read     *
//    value from physical value to application defined values                                      *
//**************************************************************************************************


std::vector<RetroRadioInput *> RetroRadioInput::_inputList;

//**************************************************************************************************
// Class: RetroRadioInput.Constructor                                                              *
//**************************************************************************************************
//  - Each application has an ID that remains constant during object lifetime (though application  *
//    constructing the object is responsible for checking the validity/uniqueness of that id if    *
//    so desired).                                                                                 *
//  - Each object also has a name, that is set equal to id at construction but can be changed      *
//    by application. That name is used to generate events at changes on input, again the          *
//    application layer is responsible for validity.                                               *
//**************************************************************************************************
RetroRadioInput::RetroRadioInput(const char* id): 
/*  
  _show(0), 
  _lastShowTime(0), 
  _lastInputType(NONE), 
  _reader(NULL),  
  _mode(0),
  _onChangeEvent(NULL), 
  _on0Event(NULL), 
  _onNot0Event(NULL),
  _id(NULL), 
  _delta(1), 
  _step(0), 
  _fastStep(0), 
  _clickEvents(0),
  _clickState(0), 
  _clickStateTime(0) 
*/  
    _show(0), _lastShowTime(0),
    _delta(1), _step(0), _fastStep(0), _mode(0),
    _lastInputType(NONE), _id(NULL),
    _onChangeEvent(NULL), _on0Event(NULL), _onNot0Event(NULL),
    _clickEvents(0), _clickState(0),
    _clickStateTime(0),
    _reader(NULL)
  
  
  {
  _maximum = _lastRead = _lastUndebounceRead = INT32_MIN;                 // _maximum holds the highest value ever read, _lastRead the
  // last read value (after mapping. Is below 0 if no valid read so far)
  setId(id);
  _inputList.push_back(this);
  for (int i = 0;i < 6;i++)
    _onClickEvent[i] = NULL;
  //setParameters("@/input");
  _timing[0] = TIME_DEBOUNCE;                // DEBOUNCE
//  else if (timingName == strstr(timingName, "cli"))
  _timing[1] = TIME_CLICK;
//  else if (timingName == strstr(timingName, "lon"))
  _timing[2] = TIME_LONG;
//  else if (timingName == strstr(timingName, "rep"))
  _timing[3] = TIME_REPEAT;    

}


//**************************************************************************************************
// Class: RetroRadioInput.get(const char *id)                                                      *
//**************************************************************************************************
//  - Returns the handle to the RetroRadioInput identified by 'id'                                 *
//  - If id is not known, create new input handler and add it to the list of known handlers        *
//**************************************************************************************************
RetroRadioInput* RetroRadioInput::get(const char *id) {
  String idStr = String(id);
  chomp(idStr);
  //idStr.toLowerCase();
  if (idStr.length() == 0)
    return NULL;
  for (int i = 0; i  < _inputList.size(); i++)
    if (strcmp(_inputList[i]->getId(), idStr.c_str()) == 0)
      return _inputList[i];
  return new RetroRadioInput(id);
}

//**************************************************************************************************
// Class: RetroRadioInput.getId()                                                                  *
//**************************************************************************************************
//  - Returns the id of the RetroRadioInput                                                        *
//**************************************************************************************************
const char* RetroRadioInput::getId() {
  return _id;
}


//**************************************************************************************************
// Class: RetroRadioInput.setId()                                                                  *
//**************************************************************************************************
//  - Sets the id of the RetroRadioInput                                                        *
//**************************************************************************************************
void RetroRadioInput::setId(const char *id) {
  if (_id)
    free(_id);
  _id = NULL;
  if (id)
    if (strlen(id))
      _id = strdup(id);
}

//**************************************************************************************************
// Class: RetroRadioInput.getEvent(String type)                                                    *
//**************************************************************************************************
//  - Returns a pointer to the name of the requested RetroRadioInput change event                  *
//  - type can be "change" (for "onchange"), "0", "not0"                                           *
//  - returns NULL if type is not valid.                                                           *
//**************************************************************************************************
char** RetroRadioInput::getEvent(String type) {
char **event = NULL;
  if (type == "change")
    event = &_onChangeEvent;
  else if (type == "0")
    event = &_on0Event;
  else if (type == "not0")
    event = &_onNot0Event;
  else if (type == "1click")
    event = &_onClickEvent[0];
  else if (type == "2click")
    event = &_onClickEvent[1];
  else if (type == "3click")
    event = &_onClickEvent[2];
  else if (type == "long")
    event = &_onClickEvent[3];
  else if (type == "1long")
    event = &_onClickEvent[4];
  else if (type == "2long")
    event = &_onClickEvent[5];
  return event;
}


//**************************************************************************************************
// Class: RetroRadioInput.getEvent(String type)                                                    *
//**************************************************************************************************
//  - Returns a pointer to the name of the requested RetroRadioInput change event                  *
//  - type can be "change" (for "onchange"), "0", "not0"                                           *
//  - returns NULL if type is not valid.                                                           *
//**************************************************************************************************
String RetroRadioInput::getEventCommands(String type, char *srcInfo) {
char **event = getEvent(type);
  if (srcInfo != NULL)
    sprintf(srcInfo, "on%s????", type.c_str());
  /*
  if (type == "change")
    event = &_onChangeEvent;
  else if (type == "0")
    event = &_on0Event;
  else if (type == "not0")
    event = &_onNot0Event;
  else if (type == "1click")
    event = &_onClickEvent[0];
  else if (type == "2click")
    event = &_onClickEvent[1];
  else if (type == "3click")
    event = &_onClickEvent[2];
  else if (type == "long")
    event = &_onClickEvent[3];
  else if (type == "1long")
    event = &_onClickEvent[4];
  else if (type == "2long")
    event = &_onClickEvent[5];
  */
  if ( NULL == event) {
    return "";
  }
  if ( NULL == *event) 
    return "";
  if ( *event[0] == '{' ) {
    if (srcInfo != NULL)
      strcpy(srcInfo, "(inline)");
    return String(*event + 1);
  }
  else 
    if (srcInfo == NULL)
      return ramsearch(*event) ? ramgetstr(*event) : nvsgetstr(*event);
  if (ramsearch(*event)) {
    sprintf(srcInfo, "(RAM-key: '%s' )", *event);
    return ramgetstr(*event);
  }
  if (nvssearch(*event)) {
    sprintf(srcInfo, "(NVS-key: '%s')", *event);
    return nvsgetstr(*event);
  }
  sprintf(srcInfo, "(undefined key: %s)", *event);
  return "";  
}



//**************************************************************************************************
// Class: RetroRadioInput.setEvent(String type, const char* content)                               *
//**************************************************************************************************
//  - Sets one of the (change) events of the RetroRadioInput object.                               *
//  - type can be "change" (for "onchange"), "0", "not0"                                           *
//  - type can be "1click", "2click", "3click", "long", "1long", "2long" for "on1click".."on2long" *
//**************************************************************************************************
void RetroRadioInput::setEvent(String type, const char* name) {
  char **event = NULL;
  char c = type.c_str()[0];
  bool clickEvent = false;
  event = getEvent(type);
  if (event)
    clickEvent = strchr( "123l", c) != NULL;  
  if (event) {
    if (*event) {
      free(*event);
      if (clickEvent)
        _clickEvents--;
    }
    *event = NULL;
    if (name)
      if (strlen(name)) {
        *event = strdup(name);
        if (clickEvent)
          _clickEvents++;
      }
  }
}


//**************************************************************************************************
// Class: RetroRadioInput.setEvent(String type, String* content)                                   *
//**************************************************************************************************
//  - Sets one of the (change) events of the RetroRadioInput object.                               *
//  - type can be "change" (for "onchange"), "0", "not0"                                           *
//  - type can be "1click", "2click", "3click", "long", "1long", "2long" for "on1click".."on2long" *
//**************************************************************************************************
void RetroRadioInput::setEvent(String type, String content) {
  if ( content.c_str()[0] == '{' )
  {
    String left, right;
    parseGroup(content, left, right);
    content = String( '{' ) + left;
  }
  setEvent(type, content.c_str());
}




//**************************************************************************************************
// Class: RetroRadioInput.setParameters(String value)                                              *
//**************************************************************************************************
//  - Sets one or more parameters of the Input objects                                             *
//  - Multiple parameters can be seperated by ','                                                  *
//  - If a parameter starts with '@' it is considered to reference an NVS-key (w/o leading '@') to *
//    read parameter(s) from                                                                       *
//**************************************************************************************************
void RetroRadioInput::setParameters(String params) {
  static int recursion = 0;
  /*
    doprint("Set Parameters!");
    doprint("Parameter id=%s", getId());
    doprint("Parameter value=%s", params.c_str());
    doprint("SetParameters for input%s=%s", getId(), params.c_str());
  */
  if (recursion > 5)
    return;
  recursion++;
  chomp(params);
  while (params.length()) {                             // anything left
    String value = split(params, ',', true);    // separate next parameter
    int ivalue;
    String param = split(value, '=');           // see if value is set
    if (strchr("@&.", param.c_str()[0])) {               // reference to NVS or RAM?
      chomp_nvs(param); 
      setParameters(param.c_str());                     // new version...
      // old version... setParameters(nvsgetstr(param.c_str() + 1));             // if yes, read NVS from key
    } else {
//      value.toLowerCase();
//      if ((value.c_str()[0] == '@') || (param != "src"))
      if (strchr("@&.", param.c_str()[0])) {               // reference to NVS or RAM?        
        chomp_nvs(value);                                 // new version
      }
      else
        chomp(value);                                     // chompNVS would destroy ~ for src link...
      //old chomp(value);
      //old value.toLowerCase();
      //old if (value.c_str()[0] == '@')
      //old   value = nvsgetstr(value.c_str() + 1);
      ivalue = atoi(value.c_str());                       // usally value is an int number
      if (param.length() > 0)
        setParameter(param, value, ivalue);
    }
  }
  recursion--;
}


void RetroRadioInput::doClickEvent(const char* type, int param)  {
//char *event = *getEvent(type);  
  String commands = getEventCommands(type);
  if (_show)
    doprint ( "Click event for Input in.%s: %s (Parameter: %d)", getId(), type, param);
  if ( commands.length() > 0 ) {
//      String commands = ramsearch(event) ? ramgetstr(event) : nvsgetstr(event);
//      Serial.printf("OnClickCommand before chomp: %s\r\n", commands.c_str());
      substitute(commands, String(param).c_str());
      if (_show)
        doprint("in.%s on%s='%s'", getId(), type, commands.c_str());
      if ( commands.length() > 0 )
        //analyzeCmdsRR ( commands );
        cmd_backlog.push_back(commands);
    }
}

#define CLICK_IDLE          0       // Not pressed so far
//#define CLICK_DEBOUNCE      1       // Pressed after idle, wait to debounce
#define CLICK_PRESS1        2       // Pressed, wait for release (or longPress)       
#define CLICK_PRESS1DONE    3       // Released after short press, wait if any other press is seen after this release
//#define CLICK_PRESS1NEXTDEB 4       // Another click detected after click1. Wait if this can be debounced
#define CLICK_PRESS2        5       // Pressed again after 1click, wait for release (or longPress)       
#define CLICK_PRESS2DONE    6       // Released after 2click short press, wait if any other press is seen after this release
//#define CLICK_PRESS2NEXTDEB 7       // Another click detected after click2. Wait if this can be debounced
#define CLICK_PRESS3        8       // Pressed after 2click debounce, wait for release (or longPress)       
#define CLICK_LONG          9       // longpress detected
#define CLICK_1LONG         10      // longpress after short click detected
#define CLICK_2LONG         11      // longpress after double click detected
//#define TIME_LONG          300

void RetroRadioInput::runClick(bool pressed) {
  if ((0 == _clickEvents) || (NONE == _lastInputType))
  {
    _clickState = CLICK_IDLE;
  }
  else if (CLICK_IDLE == _clickState) 
  {
    if (pressed) 
    {
      _clickStateTime = millis();
//      _clickState = CLICK_DEBOUNCE;
      _clickState = CLICK_PRESS1;
    }
  }
/*
  else if (CLICK_DEBOUNCE == _clickState) 
  {
    if (!pressed)
      _clickState == CLICK_IDLE;
    else if (millis() - _clickStateTime > _debounceTime) 
    {
      _clickStateTime = millis();
      _clickState = CLICK_PRESS1;
    }
  }
*/  
  else if (CLICK_PRESS1 == _clickState)
  {
    if (!pressed) {
      _clickState = CLICK_PRESS1DONE;
      _clickStateTime = millis();
    } 
    else if (millis() - _clickStateTime > _timing[T_LON_IDX])
    {
      _clickStateTime = millis();// - TIME_LONG;
      _clickState = CLICK_LONG;
      _clickLongCtr = 1;
      doClickEvent("long", _clickLongCtr++);
    }
  }
  else if (CLICK_LONG == _clickState)
  {
    if (millis() - _clickStateTime >= _timing[T_REP_IDX])
    {
      doClickEvent("long", _clickLongCtr++);
      _clickStateTime = millis();
    }
    if (!pressed) {
      doClickEvent("long");
      _clickState = CLICK_IDLE;
    } 
  }
  else if (CLICK_PRESS1DONE == _clickState)
  {
    if (!pressed ) 
    {
      if (millis() - _clickStateTime > _timing[T_CLI_IDX])
      {
        doClickEvent("1click");
        _clickState = CLICK_IDLE;
      }
    }
    else
    {
      _clickStateTime = millis();
//      _clickState = CLICK_PRESS1NEXTDEB;
      _clickState = CLICK_PRESS2; 
    }
  }
/*
  else if (CLICK_PRESS1NEXTDEB == _clickState) 
  {
    if (!pressed)
      _clickState == CLICK_IDLE;
    else if (millis() - _clickStateTime > _debounceTime) 
    {
      _clickStateTime = millis();
      _clickState = CLICK_PRESS2;
    }
  }
*/  
  else if (CLICK_PRESS2 == _clickState)
  {
    if (!pressed) {
      _clickState = CLICK_PRESS2DONE;
      _clickStateTime = millis();
    } 
    else if (millis() - _clickStateTime > _timing[T_LON_IDX])
    {
      _clickStateTime = millis();// - TIME_LONG;
      _clickState = CLICK_1LONG;
      _clickLongCtr = 1;
      doClickEvent("1long", _clickLongCtr++);
    }
  }
  else if (CLICK_1LONG == _clickState)
  {
    if (millis() - _clickStateTime >= _timing[T_REP_IDX])
    {
      doClickEvent("1long", _clickLongCtr++);
      _clickStateTime = millis();
    }
    if (!pressed) {
      doClickEvent("1long");
      _clickState = CLICK_IDLE;
    } 
  }
  else if (CLICK_PRESS2DONE == _clickState)
  {
    if (!pressed) 
    {
      if (millis() - _clickStateTime > _timing[T_CLI_IDX])
      {
        doClickEvent("2click");
        _clickState = CLICK_IDLE;
      }
    }
    else
    {
      _clickStateTime = millis();
//      _clickState = CLICK_PRESS2NEXTDEB; 
      _clickState = CLICK_PRESS3; 
    }
  }
/*
  else if (CLICK_PRESS2NEXTDEB == _clickState) 
  {
    if (!pressed)
      _clickState == CLICK_IDLE;
    else if (millis() - _clickStateTime > _debounceTime) 
    {
      _clickStateTime = millis();
      _clickState = CLICK_PRESS3;
    }
  }
*/
  else if (CLICK_PRESS3 == _clickState)
  {
    if (!pressed) 
    {
        doClickEvent("3click");
        _clickState = CLICK_IDLE;
    } 
    else if (millis() - _clickStateTime > _timing[T_LON_IDX])
    {
      _clickStateTime = millis();// - TIME_LONG;
      _clickState = CLICK_2LONG;
      _clickLongCtr = 1;
      doClickEvent("2long", _clickLongCtr++);
    }
  }
  else if (CLICK_2LONG == _clickState)
  {
    if (millis() - _clickStateTime >= _timing[T_REP_IDX])
    {
      doClickEvent("2long", _clickLongCtr++);
      _clickStateTime = millis();
    }
    if (!pressed) {
      doClickEvent("2long");
      _clickState = CLICK_IDLE;
    } 
  }
}

//**************************************************************************************************
// Class: RetroRadioInput.setTiming(const char* timingName, int32_t ivalue)                        *
//**************************************************************************************************
//  - Set one of the timings (identified by timingName) of the Input                               *
//  - only first 3 chars are significant, the starting "t-" is already removed by caller           *
//    - "deb"ounce: debounce (def: 0)                                                              *
//    - "cli"ck: how long to wait after 1./2. click if 2./3. click (or longpr.) follows  (def. 300)*
//    - "lon"g: time for the first longpress (def. 500)                                            *
//    - "rep"eat: time for all longpress after the first (def. 500)                                *
//  - all times in ms                                                                              *
//  - default times are used in constructor only, not for the empty-String assignment ("t-long="   *
//    will set "lon"g-timing to 0)                                                                 *     
//**************************************************************************************************
void RetroRadioInput::setTiming(const char* timingName, int32_t ivalue) {
  if (ivalue < 0)
    ivalue = 0;
  if (timingName == strstr(timingName, "deb"))
    _timing[0] = ivalue;
  else if (timingName == strstr(timingName, "cli"))
    _timing[1] = ivalue;
  else if (timingName == strstr(timingName, "lon"))
    _timing[2] = ivalue;
  else if (timingName == strstr(timingName, "rep"))
    _timing[3] = ivalue;    
}


//**************************************************************************************************
// Class: RetroRadioInput.setParameter(String param, String value, int32_t ivalue)                 *
//**************************************************************************************************
//  - Actually sets one parameter                                                                  *
//  - param and value are no further processed (no expansion of NVS etc...)                        *
//  - ivalue is the int32_t representation of value                                                *
//  - virtual, can be overidden by subclasses                                                      *
//**************************************************************************************************
void RetroRadioInput::setParameter(String param, String value, int32_t ivalue) {
  ////
  //dbgprint("PARAM: %s, VALUE: %s", param.c_str(), value.c_str());
  param.toLowerCase();
  if (param == "show") {
    _show = (uint32_t)ivalue * 1000UL;
  } else if (param == "map") {                        // set the valueMap for the input
    setValueMap(value);                               // see RetroRadioInput::setValueMap() for details
  } else if (param == "map+") {                       // extent the valueMap for the input
    setValueMap(value, true);                         // see RetroRadioInput::setValueMap() for details
  } else if ((param == "event") || (param == "onchange")) {                      // set the change event
    setEvent("change", value);                       // on mapped input, if not hit, nearest valid value will be used
    //  } else if (param == "calibrate") {                  // as "show", just more details on map hits.
    //    _calibrate = ivalue;
  } else if (param.startsWith("on")) {                      // set the change event
    setEvent(param.substring(2), value);                   // on mapped input, if not hit, nearest valid value will be used
    //  } else if (param == "calibrate") {                  // as "show", just more details on map hits.
    //    _calibrate = ivalue;
//  } else if (param == "debounce") {                     // how long must a value be read before considered valid?
    //_debounceTime = ivalue;                           // (in msec).
  } else if (param == "delta") {
    if (ivalue > 1)
      _delta = ivalue;
    else
      _delta = 1;
  } else if (param == "mode") {
    _mode = ivalue;
    if (_reader)
      _reader->mode(_mode);
  } else if ((param == "nvs") || (param == "ram")) {
    if (value.length())
      if (_reader) {
        int x = _lastRead;
        if (_lastInputType == NONE) {
          x = read(false);
        }
        if (param == "nvs")
          nvssetstr(value.c_str(), String(x));
        else
          ramsetstr(value.c_str(), String(x));
      }
  } else if (param == "src") {
    setReader(value);
  } else if (param == "start") {
    _lastInputType = NONE;
    //Serial.printf("Executing Start for in.%s\r\n", getId());
    if (_reader)
      _reader->start();
    read(true);
    //Serial.printf("Start for in.%s Done!\r\n", getId());
  } else if (param == "stop") {
    runClick(false);
    if (_reader)
      _reader->stop();
    _lastInputType = NONE;
  } else if ((param == "info") || (param == "?")) {
    showInfo(true);
  } else if (param.startsWith("t-")) {
    setTiming(param.c_str()+2, ivalue);
  }
  
}

//**************************************************************************************************
// Class: RetroRadioInput::showInfo(bool hint)                                                     *
//**************************************************************************************************
//  - Displays (all) relevant settings for the Input on Serial (even if DEBUG = 0)                 *
//  - Can be used for debugging (to see if all settings are as expected), no functional impact     *
//  - Will not read the input (for this, set the "property" 'show' to a value > 0 to have cyclic   *
//    display of the readings (with distance between readings set by the value assigned to 'show'  *
//  - If 'hint' is true, an info is displayed showing all other known inputs                       *
//**************************************************************************************************
void RetroRadioInput::showInfo(bool hintToOthers) {
  char buf[100];
  doprint("Info for Input \"in.%s\":", getId());
  if (_reader == NULL)
    doprint(" * No source linked, Input not operational!");
  else 
  {
    //if (_lastInputType == NONE)
    if (_reader->started())
      doprint(" * Input is started/polling");
    else
      doprint(" * Input is not started/not polling");
    doprint(" * Src: %s", _reader->info().c_str());
    if (0 != _reader->type())
    {
      _reader->specialInfo();
    }
    else
    {
      if (_valueMap.size() >= 4) {
        doprint(" * Value map contains %d entries:", _valueMap.size() / 4);
        for (int i = 0; i < _valueMap.size(); i += 4)
          doprint("    %3d: (%d..%d = %d..%d)", 1 + i / 4,
                  _valueMap[i], _valueMap[i + 1], _valueMap[i + 2], _valueMap[i + 3]);
      } else
        doprint(" * Value map is NOT set!");
      doprint(" * Delta: %d", _delta);
      doprint(" * Show: %d (seconds)", _show / 1000);
      doprint(" --Event-Info--");
      if (!hasChangeEvent()) {
        doprint(" * there are no change-event(s) defined.");
      } 
      else 
        for (int i = 0; i < 3; i++) 
        {
          
          const char* type = (i == 0) ? "change" : (i == 1 ? "0" : "not0");
          String cmnds = getEventCommands(type, buf);
          if (cmnds.length() > 0) 
          {
            doprint(" * on%s-event: \"%s\" %s", type, cmnds.c_str(), buf);
    /*        
            if (ramsearch(*cmnds)) {
              String commands = ramgetstr(*cmnds);
              //chomp_nvs(commands);
              doprint("    defined in RAM as: \"%s\"", commands.c_str());
            } else if (nvssearch(*cmnds)) {
              String commands = nvsgetstr(*cmnds);
              //chomp_nvs(commands);
              doprint("    defined in NVS as: \"%s\"", commands.c_str());
            }  else
              doprint("    NOT DEFINED! (Neither NVS nor RAM)!");
          }
    */  
          }
        }
      if (_clickEvents == 0)  
        doprint( " * There are no click-event(s) defined." );  
      else 
      {
        char type[10];
        strcpy(type, "xclick");
        for (int i = 0; i < 3; i++) 
        {
          type[0] = '1' + i;
          String cmnds = getEventCommands(type, buf);
          if (cmnds.length() > 0) 
          {
            doprint(" * on%s-event: \"%s\" %s", type, cmnds.c_str(), buf);
    /*
            if (ramsearch(*cmnds)) {
              String commands = ramgetstr(*cmnds);
              doprint("    defined in RAM as: \"%s\"", commands.c_str());
            } else if (nvssearch(*cmnds)) {
              String commands = nvsgetstr(*cmnds);
              doprint("    defined in NVS as: \"%s\"", commands.c_str());
            }  else
              doprint("    NOT DEFINED! (Neither NVS nor RAM)!");
          }
    */   
          }   
        }
        strcpy(type, "long");
        String cmnds = getEventCommands(type, buf);
        if (cmnds.length() > 0) 
        {
          doprint(" * on%s-event: \"%s\" %s", type, cmnds.c_str(), buf);
    /*
          if (ramsearch(*cmnds)) {
            String commands = ramgetstr(*cmnds);
            doprint("    defined in RAM as: \"%s\"", commands.c_str());
          } else if (nvssearch(*cmnds)) {
            String commands = nvsgetstr(*cmnds);
            doprint("    defined in NVS as: \"%s\"", commands.c_str());
          }  else
            doprint("    NOT DEFINED! (Neither NVS nor RAM)!");
    */  
        }
      
        strcpy(type, "xlong");
        for (int i = 0; i < 2; i++) 
        {
          type[0] = '1' + i;
          String cmnds = getEventCommands(type, buf);
          if (cmnds.length() > 0) 
          {
            doprint(" * on%s-event: \"%s\" %s", type, cmnds.c_str(), buf);
    /*
            if (ramsearch(*cmnds)) {
              String commands = ramgetstr(*cmnds);
              doprint("    defined in RAM as: \"%s\"", commands.c_str());
            } else if (nvssearch(*cmnds)) {
              String commands = nvsgetstr(*cmnds);
              doprint("    defined in NVS as: \"%s\"", commands.c_str());
            }  else
              doprint("    NOT DEFINED! (Neither NVS nor RAM)!");
          }    
        }
    */
          }
        }
      }
      doprint(" --Timing-Info--");
      doprint(" * t-debounce: %5ld ms", _timing[T_DEB_IDX]);
      doprint(" * t-click   : %5ld ms (wait after short click)", _timing[T_CLI_IDX]);
      doprint(" * t-long    : %5ld ms (detect first longpress)", _timing[T_LON_IDX]);
      doprint(" * t-repeat  : %5ld ms (repeated longpress)", _timing[T_REP_IDX]);
      
    //  doprint(" * Debounce: %d (ms)", _debounceTime);
    }
  }
  if (hintToOthers)
    if (_inputList.size() > 1) 
    {
      doprint("There are %d more Inputs defined:", _inputList.size() - 1);
      int n = 0;
      for (int i = 0; i < _inputList.size(); i++)
        if (_inputList[i] != this)
          doprint(" %2d: \"in.%s\"", ++n, _inputList[i]->getId());
    }
}

//**************************************************************************************************
// Class: RetroRadioInput::setValueMap(String value, bool extend)                                  *
//**************************************************************************************************
//  - Set the mapping table of the RetroRadioInput object                                          *
//  - If extend is true, the existing map is extended (default = false will clear existing map)    *
//  - more than one mapping entry can be added, each entry must be surrounded by '( )'             *
//  - if value (after chomp) starts with @ the remaining string is treated as key to an NVS entry  *
//    from preferences or RAM entry if no NVS entry is associated with that key                    *
//  - One single entry is expected to have the format:                                             *
//        x1[..x2] = y1[..y2]                                                                      *
//    Interpretation: if read is called, the phyical value obtained from physRead()                *
//    is compared with all defined map-entries if it fits within x1 and x2 (x2 can be below x1)    *
//    x1 and x2 are inclusive, if x2 is not provided it will be set equal to x1.                   *
//    If there is a hit, x will be mapped to y1 to y2. (if y2 is not provided, y2 will set to y1)  *
//    both x and y range can be in reverse order (but for y must not be below 0)                   *
//    Examples:                                                                                    *
//       (0 = 1) (1 = 0) (expanded to (0..0 = 1..1)(1..1 = 0..0) will iverse a digital input       *
//       (0..1 = 1..0) would do the same with just one map entry                                   *
//       0, 4095 = 100, 0  will convert input values between 0-4095 (i. e. from analog input) to   *
//                 an output range of 100 to 0                                                     *
//  - A map entry with empty left part is also possible. In that case the left part will be set to *
//    x1 = INT32_MIN and x2 = INT32_MAX. This cancels also the map setting, as this entry will     *
//    cover the full possible input range.                                                         *
//  - In case of overlapping map ranges, the first defined map enty will fire. Example:            *
//    (1000..3000 = 1)(=2)(4000..4050=3)                                                           *
//    will yield 1 for all readings between 1000 and 3000 (inclusive), and 2 in any other case     *
//    (the third entry will be ignored since the range between 4000 and 4050 is also covered by    *
//    the second
//  - calling setMap will erase the old mapping                                                    *
//**************************************************************************************************
void RetroRadioInput::setValueMap(String value, bool extend) {
  chomp_nvs(value);
  value.toLowerCase();
  if (!extend)
    _valueMap.clear();
  while (value.length() > 0) {
    String mapEntryLeft, mapEntryRight;
    const char *delimiters[] = {"=", NULL};
    int idx = parseGroup(value, mapEntryLeft, mapEntryRight, delimiters);
    if (idx == 0) {
      const char *delimiters[] = {"..", NULL};
      String from1, from2, to1, to2;
      if (mapEntryLeft.length() == 0) {
        from1 = String(INT32_MIN);
        from2 = String(INT32_MAX);
        //Serial.printf("From: %s, to: %s\r\n", from1.c_str(), from2.c_str());
      }
      else if (-1 == parseGroup(mapEntryLeft, from1, from2, delimiters, true))
        from2 = from1;
      if (-1 == parseGroup(mapEntryRight, to1, to2, delimiters, true))
        to2 = to1;
      _valueMap.push_back(from1.toInt());                  // add to map if both x and y pair is valid
      _valueMap.push_back(from2.toInt());
      _valueMap.push_back(to1.toInt());
      _valueMap.push_back(to2.toInt());
    }
  }
}


class RetroRadioInputReaderDigital: public RetroRadioInputReader {
  public:
    RetroRadioInputReaderDigital(int pin, int mod) {
      _pin = pin;
      mode(mod);
    };
    int32_t read(int8_t& forceMode) {
      (void)forceMode;
      return _inverted ? !digitalRead(_pin) : digitalRead(_pin);
    };
    void mode(int mod) {
      _mode = mod;
      _inverted = 0 != (mod & 0b1);
      if (0 != (mod & 0b10))
        pinMode(_pin, INPUT_PULLUP);
      else
        pinMode(_pin, INPUT);
    }
    String info() {
      return String("Digital Input, pin: ") + _pin +
             ", Inverted: " + _inverted + ", PullUp: " + (0 != (_mode & 0b10));
    }

  private:
    int _pin;
    bool _inverted;
};

class RetroRadioInputReaderRam: public RetroRadioInputReader {
  public:
    RetroRadioInputReaderRam(const char *key): _key(NULL) {
      if (key)
        if (strlen(key))
          _key = strdup(key);
      _lastKnown = false;
    };
    int32_t read(int8_t& forceMode) {
      int32_t res = 0;
      if (ramsearch(_key))
      {
        res = atoi(ramgetstr(_key).c_str());
        _lastKnown = true;
      }
      else
      {
        forceMode = -1;
        _lastKnown = false;
      }
      return res;
    };
    void mode(int mod) {
    }
    String info() {
      return String("RAM Input, key: ") + (_key ? _key : "<null>");
    }
  protected:
    void deleteContent() {
      if (_key)
        free((void *)_key);
      _key = NULL;
    }

  private:
    const char *_key;
    bool _lastKnown;
};


class RetroRadioInputReaderAnalog: public RetroRadioInputReader {
  public:
    RetroRadioInputReaderAnalog(int pin, int mod) {
      _pin = pin;
      _last = -1;
      _filter = 0;
      mode(mod);
    };
    void mode(int mod) {
      _mode = mod;
      _inverted = 0 != (mod & 0b1);
      if (0 != (mod & 0b10))
        pinMode(_pin, INPUT_PULLUP);
      else
        pinMode(_pin, INPUT);
      _filter = (0b11100 & mod) >> 2;
    }
    int32_t read(int8_t &forceMode) {
      (void)forceMode;
      uint32_t t = millis();
      if (_last >= 0)
        if ((t - _lastReadTime) < 20)
          return (_inverted ? (4095 - _last) : _last);
      _lastReadTime = t;
      int x = analogRead(_pin);
      //   Serial.printf("AnalogRead(%d) = %d\r\n", _pin, x);
      if ((_last >= 0) && (_filter > 0)) {
        _last = (_last * _filter + x) / (_filter + 1);
      /*
        if (_last < x)
          _last++;
        else if (_last > x)
          _last--;
      */
      }
      else
        _last = x;
      return (_inverted ? (4095 - _last) : _last);
    };

    String info() {
      return String("Analog Input, pin: ") + _pin +
             ", Inverted: " + _inverted + ", PullUp: " + (0 != (_mode & 0b10)) + ", Filter: " + (7 & (_mode >> 2));
    }
  private:
    int _pin, _last, _filter;
    bool _inverted;
    uint32_t _lastReadTime;
};


class RetroRadioInputReaderTouch: public RetroRadioInputReader {
  public:
    RetroRadioInputReaderTouch(int pin, int mod) {
      _pin = pin;
      _auto = false;
      touch_pad_config((touch_pad_t)pin, 0);
      int i = 0;
      uint16_t x = 0;

      while ((i++ < 100) && (x == 0)) {   // Workaround: touch_pad_read_filtered() needs some time to settle... (will return 0 else)
        touch_pad_read_filtered((touch_pad_t)_pin, &x);
        delay(1);
      }
      if (pin == 8)
        Serial.printf("TouchRead Nr. %d, T%d=%d\r\n", i, pin, x);
      mode(mod);
    };
    void mode(int mod) {
      bool wasAuto = _auto;
      _digital = (0 != (mod & 0b1));
      if (_digital)
        _auto = true;
      else
        _auto = (0 != (mod & 0b10));
      if (_auto && !wasAuto)
        _maxTouch = _minTouch = 50;
    }

    int32_t read(int8_t& forceMode) {
      uint16_t x;
      (void)forceMode;
      touch_pad_read_filtered((touch_pad_t)_pin, &x);
      if (x == 0) {
        return _digital ? 1 : 1023;
      }
      if (_auto) {
        if (x >= _maxTouch) {
          if (_minTouch == _maxTouch)
            if (0 > (_minTouch = x - 50)) {
              return _digital ? 1 : 1023;
            }
          _maxTouch = x;
        }
        else if (_maxTouch > _minTouch) {
          if (_minTouch > x)
            _minTouch = x;
          if (_auto && (_maxTouch - x < 5) && (_maxTouch > 10))
            _maxTouch--;
        }

        x = map(x, _minTouch, _maxTouch, 0, 1023);
        return _digital ? (x >= 800) : x;
      }
      else
        return x;
    };
    String info() {
      return String("Touch Input: T") + _pin + ", Digital use: " + _digital +
             ", Auto: " + _auto + (_auto ? " (auto calibration to 1023 if not touched)" : " (pin value is used direct w/o calibration)") ;
    }
  private:
    int _pin;
    bool _auto, _digital;
    int16_t _maxTouch, _minTouch;
};

class RetroRadioInputReaderSystem: public RetroRadioInputReader {
  public:
    RetroRadioInputReaderSystem(const char* reference): _ref(NULL), _refname(NULL), _readf(NULL) {
      for (int i = 0; (_readf == NULL) && (i < KNOWN_SYSVARIABLES); i++)
      {
        //dbgprint("%d: strcmp(%s, %s) = %d", i, reference, sysVarReferences[i].id, strcmp(reference, sysVarReferences[i].id));
        if (0 == strcmp(reference, sysVarReferences[i].id))
          if (NULL != sysVarReferences[i].readf) {
            _ref = sysVarReferences[i].ref;
            _readf = sysVarReferences[i].readf;
            _refname = sysVarReferences[i].id;
            _volatile = true;
          }
      }
      if (NULL == _refname)
        _refname = strdup(reference);
    };

    void mode(int mod) {
    }

    int32_t read(int8_t& forceMode) {
      (void)forceMode;
      if (_readf)
        return _readf(_ref).toInt();
      return 0;
    };

    String info() {
      String ret;
      if (!_readf)
        ret = "Set to unknown ";
      ret = ret + "Variable: " + _refname;
      if (!_readf)
        ret += " (will always read as 0)";
      return ret;   
    }
  private:
    void *_ref;
    const char* _refname;
    String (*_readf)(void *);
};



void RetroRadioInput::setReader(String value) {
  RetroRadioInputReader *reader = NULL;
  chomp(value);
  //value.toLowerCase();
  int idx = value.substring(1).toInt();
  //dbgprint("SetReader, value = %s\r\n", value.c_str());
  if ((value.c_str()[0] == '@') || (value.c_str()[0] == '&'))
  {
    chomp_nvs(value);
    //value.toLowerCase();
    if (value.length() == 0)
      value = "-1";
    idx = value.substring(1).toInt();
    //Serial.printf("Source of input after chomp nvs is: %s\r\n", value.c_str());
  }
  char c;
  switch (c = tolower(value.c_str()[0])) {
    case 'd': 
      reader = new RetroRadioInputReaderDigital(idx, _mode);
      break;
    case 'a': 
       if ((_reader == NULL) && (_mode == 0))
        _mode = 0b11100;
      reader = new RetroRadioInputReaderAnalog(idx, _mode);
      break;
    case 't': 
      reader = new RetroRadioInputReaderTouch(idx, _mode);
      break;
    case '.': 
      reader = new RetroRadioInputReaderRam(value.c_str() + 1);
      _timing[T_DEB_IDX] = 0;
      break;
    case '~': reader = new RetroRadioInputReaderSystem(value.c_str() + 1);
      _timing[T_DEB_IDX] = 0;
      break;
    case 'm': 
    //case 'M':
        value = value.substring(1);
        value.trim();
        reader = new RetroRadioInputReaderMqtt(this, value.c_str());
      break;    
    case '0':
      if (_reader)
        delete _reader;
      _reader = NULL;
      reader = NULL;
      break;
    case '-': 
      if (idx == 1)
        if (_reader)
        {
          delete _reader;
          _reader = NULL;
          reader = NULL;
        }  
      break;
  }
  if (reader) {
    if (_reader)
      delete _reader;
    _reader = reader;
    _lastInputType = NONE;
    Serial.printf("Reader %lX set for %lX!", (unsigned long)_reader, (unsigned long)this);
  }
}



//**************************************************************************************************
// Class: RetroRadioInput.physRead()                                                               *
//**************************************************************************************************
//  - Virtual method, must be overidden by subclasses. Returns an int16 as current read value.     *
//  - Invalid, if below 0                                                                          *
//**************************************************************************************************
int32_t RetroRadioInput::physRead(int8_t& forceMode) {
  if (!_reader) {
    _lastInputType = NONE;
    forceMode = -1;
    return 0;
  } else if (0 != _reader->type() ) {
    _lastInputType = NONE;
    forceMode = -1;
    return 0;
  }
  else {
    if (_lastInputType == NONE)
      _lastInputType = HIT;
    return _reader->read(forceMode);
  }
}

//**************************************************************************************************
// Class: RetroRadioInput.Destructor                                                               *
//**************************************************************************************************
RetroRadioInput::~RetroRadioInput() {
  if (_id)
    free(_id);
  if (_onChangeEvent)
    free(_onChangeEvent);
  if (_on0Event)
    free(_on0Event);
  if (_onNot0Event)
    free(_onNot0Event);
  if (_reader)
    delete _reader;
};

int32_t RetroRadioInput::read(bool doStart) {
  if (_reader)
    if (_reader->type() != 0)
      return 0;
  int8_t forced = (_lastInputType == NONE)?1:0;
  bool show = (_show > 0) && ((millis() - _lastShowTime > _show) || (1 == forced));
  String showStr;
  int32_t x = physRead(forced);
  int32_t nearest = 0;
  if (show)
    _lastShowTime = millis();
  if (NONE == _lastInputType) {
    if (show)
      doprint("Input \"in.%s\" fail: no source associated", getId());
    return 0;
  } else if (show) {
    char xbuf[10];
    sprintf(xbuf, "%5d", x);
    showStr = String("Input \"in.") + getId() + "\" (is " + (doStart ? "running" : "stopped") + ") physRead=" + xbuf;
  }
  if (forced < 0)
    return x;

  bool found = true;
  if (_valueMap.size() >= 4) {
    size_t idx = 0;
    uint32_t maxDelta = UINT32_MAX;
    found = false;
    //if (show) Serial.printf("\r\nVALUE-MAP-SIZE: %d\r\n", )

    while (idx < _valueMap.size() && !found) {
      int32_t c1, c2;
      int32_t x1, x2, y1, y2;
      c1 = x1 = _valueMap[idx++];
      c2 = x2 = _valueMap[idx++];
      y1 = _valueMap[idx++];
      y2 = _valueMap[idx++];
      //c1 = x1 = (x1 < 0 ? _maximum + x1 : x1);
      //c2 = x2 = (x2 < 0 ? _maximum + x2 : x2);
      if (c1 > c2) {
        int32_t t = c1;
        c1 = c2;
        c2 = t;
      }
      if ((x >= c1) && (x <= c2)) {
        found = true;
        if (y1 == y2)
          x = y1;
        else
          x = map(x, x1, x2, y1, y2);
      } else {
        int32_t newNearest;
        uint32_t myDelta = x < c1 ? c1 - x : x - c2;
        if (x < c1) {
          //newNearest = -y1 - 1;
          newNearest = y1;
          myDelta = c1 - x;
        } else {
          //newNearest = -y2 - 1;
          newNearest = y2;
          myDelta = x - c2;
        }
        if (myDelta < maxDelta) {
          maxDelta = myDelta;
          nearest = newNearest;
        }
      }
    };
  }
  if (found) {
    if (show && (_valueMap.size() >= 4)) {
      char xbuf[10];
      sprintf(xbuf, "%5d", x);
      showStr = showStr + " ( mapped to:" + xbuf + ")";
    }
    _lastInputType = HIT;
  }
  else {
    if (show && (_valueMap.size() >= 4)) {
      char xbuf[10];
      sprintf(xbuf, "%5d", nearest);
      showStr = showStr + " (nearest is:" + xbuf + ")";
    }
    if (forced) {
      x = nearest;
      _lastInputType = NEAREST;
      /*      if (show) {// (_show > 0)
              char xbuf[10];
              sprintf(xbuf, "%5d)", x);
              showStr = showStr + " (mapped to nearest=" + xbuf;
            }
      */
    } else
      x = _lastRead;
  }
  int32_t lastRead = 0;
  if (show)
    doprint(showStr.c_str());
  if (!doStart) {
    if (forced)
      _lastInputType = NONE;
    forced = false;
  } else if (forced) {
    _lastRead = _lastUndebounceRead = x;
    _lastReadTime = millis();
  } else {
    if (x != _lastUndebounceRead) {
      _lastUndebounceRead = x;
      _lastReadTime = millis();
      if (forced = (0 == _timing[T_DEB_IDX]))
        lastRead = _lastRead = x;
      else
        x = _lastRead;

    } else if (x != _lastRead) {
      if ((millis() - _lastReadTime < _timing[T_DEB_IDX]) || (_delta > abs(_lastRead - x)))
        x = _lastRead;
      else {
        lastRead = _lastRead;
        _lastRead = x;
        forced = true;
      }
    }
  }
  if (forced) {
    if (_show > 0)
      doprint("Input \"in.%s\": change to %d from %d, checking for events!", getId(), x, lastRead);
    String commands = getEventCommands("change");
    if ( commands.length() > 0 ) 
    {
      substitute(commands, String(x).c_str());
      if (_show > 0)
        doprint ( "Executing onchange: '%s'", commands.c_str() );
      //analyzeCmdsRR ( commands );
      cmd_backlog.push_back(commands);
    }
    if (x == 0) {                   // went to 0
      String commands = getEventCommands("0");
      if ( commands.length() > 0 ) 
      {
        substitute(commands, String(x).c_str());
        if (_show > 0)
          doprint ( "Executing on0: '%s'", commands.c_str() );
        //analyzeCmdsRR ( commands );
        cmd_backlog.push_back(commands);
      }
    } 
    if (_clickEvents > 0)
      runClick ( 0 == x );    
    if ((x != 0) && (lastRead == 0)) {     // comes from 0
      String commands = getEventCommands("not0");
      if ( commands.length() > 0 ) 
      {
        substitute(commands, String(x).c_str());
        if (_show > 0)
          doprint ( "Executing onnot0: '%s'", commands.c_str() );
        //analyzeCmdsRR ( commands );
        cmd_backlog.push_back(commands);

      }
    }
/*    
    if (cmnds) {
      if (_show > 0) {
        doprint("Input \"in.%s\": change to %d (event \"%s\")", getId(), x, cmnds);
      }

      String commands = ramsearch(cmnds) ? ramgetstr(cmnds) : nvsgetstr(cmnds); //nvssearch(specificEvent)?nvsgetstr(specificEvent):nvsgetstr(cmnds);
      Serial.printf("OnChangeCommand before chomp: %s\r\n", commands.c_str());
      substitute(commands, String(x).c_str());
      Serial.printf("OnChangeCommand after substitute: %s\r\n", commands.c_str());
      analyzeCmdsRR ( commands );
    }
    if (x == 0) {                   // went to 0
      cmnds = _on0Event;
      if (cmnds) {
        if (_show > 0) {
          doprint("Input \"in.%s\": became Zero (event \"%s\")", getId(), cmnds);
        }
        String commands = ramsearch(cmnds) ? ramgetstr(cmnds) : nvsgetstr(cmnds); //nvssearch(specificEvent)?nvsgetstr(specificEvent):nvsgetstr(cmnds);
        substitute(commands, String(x).c_str());
        analyzeCmdsRR ( commands );
      }
    } else if (lastRead == 0) {     // comes from 0
      cmnds = _onNot0Event;
      if (cmnds) {
        if (_show > 0) {
          doprint("Input \"in.%s\": became NonZero (event \"%s\")", getId(), cmnds);
        }
        String commands = ramsearch(cmnds) ? ramgetstr(cmnds) : nvsgetstr(cmnds); //nvssearch(specificEvent)?nvsgetstr(specificEvent):nvsgetstr(cmnds);
        substitute(commands, String(x).c_str());
        analyzeCmdsRR ( commands );
      }
    }
   }
*/
  } 
  else   
  {
    if (_clickEvents > 0)
      runClick ( 0 == x );
  }
  return x;
}

void RetroRadioInput::executeCmdsWithSubstitute(String commands, String substitute) {
  int idx = commands.indexOf('?');
  while (idx >= 0) {
    commands = commands.substring(0, idx) + substitute + commands.substring(idx + 1);
    idx = commands.indexOf('?');
  }
  analyzeCmdsRR ( commands );
}

bool RetroRadioInput::hasChangeEvent() {
  return (NULL != _onChangeEvent) || (NULL != _on0Event) || (NULL != _onNot0Event);
}

void RetroRadioInput::checkAll() {
  for (int i = 0; i < _inputList.size(); i++) {
    RetroRadioInput *p = _inputList[i];
    bool isRunning = p->_lastInputType != NONE;
    if ((isRunning && (p->hasChangeEvent() || (p->_clickEvents > 0))) || ((p->_show > 0) && (millis() - p->_lastShowTime > p->_show))) {
      p->read(isRunning);
    }
  }
}

void RetroRadioInput::showAll(const char *p) {
  int count = 0;
  if (_inputList.size() == 0)
  {
    dbgprint("There are no inputs defined!");
  }
  else
  {
    if (p)
      if (0 == strlen(p))
        p = NULL;
    if (p)
      dbgprint("Info for all Inputs with \"%s\" in their name", p);
    else
      dbgprint("Info for all known Inputs");
    for (int i = 0; i < _inputList.size(); i++) {
      bool match = (p == NULL);
      if (!match)
        match = (strstr(_inputList[i]->getId(), p) != NULL);
      if (match)
      {
        dbgprint("---------------------------------------------------------------------------");
        _inputList[i]->showInfo(false);
        count++;
      }
    }
    if (count)
      dbgprint("%d (of %d) Inputs shown", count, _inputList.size());
    else
      dbgprint("None of the known Inputs matched the criteria. Use command \"in\" (without parameter) to list all known Inputs.");
  }
}

void doInput(const char* idStr, String value) {
  RetroRadioInput *i = RetroRadioInput::get(idStr);
  if (i)
    i->setParameters(value);
}


std::map<String, String> ramContent;

//**************************************************************************************************
//                                      R A M G E T S T R                                          *
//**************************************************************************************************
// Read a string from RAM.                                                                         *
//**************************************************************************************************
String ramgetstr ( const char* key )
{
  auto search = ramContent.find(String(key));
  if (search == ramContent.end())
    return String("");
  else
    return search->second;
}


//**************************************************************************************************
//                                      R A M S E T S T R                                          *
//**************************************************************************************************
// Put a key/value pair in RAM.  Length is limited to allow easy read-back.                        *
//**************************************************************************************************
void ramsetstr ( const char* key, String val )
{
  ramContent[String(key)] = val;
}

//**************************************************************************************************
//                                      R A M S E A R C H                                          *
//**************************************************************************************************
// Check if key exists in RAM.                                                                     *
//**************************************************************************************************
bool ramsearch ( const char* key )
{
  if (key)
  {
    auto search = ramContent.find(String(key));
    // if (search == ramContent.end()) Serial.printf("RAM search failed for key=%s\r\n", key);
    return (search != ramContent.end());
  }
  return false;
}

//**************************************************************************************************
//                                      R A M D E L K E Y                                          *
//**************************************************************************************************
// Delete a keyname in RAM.                                                                        *
//**************************************************************************************************
void ramdelkey ( const char* key)
{
  auto search = ramContent.find(String(key));
  dbgprint("RamDelKey(%s)=%d", key, search != ramContent.end());
  if (search != ramContent.end())
    ramContent.erase(search);
}


//**************************************************************************************************
//                                      D O R A M L I S T                                          *
//**************************************************************************************************
// - command "ramlist": list the full RAM content to Serial (even if DEBUG = 0)                    *
// - if parameter p != NULL, only show entries which have *p as substring in keyname               *
//**************************************************************************************************
String doRamlist(const char* p) {
  String ret = "{\"ramlist\"=[";
  bool first = true;
  doprint("RAM content (%d entries)", ramContent.size());
  auto search = ramContent.begin();
  int i = 0;
  while (search != ramContent.end()) {
    bool match = (p == NULL);
    if (!match) 
      match = (NULL != strstr(search->first.c_str(), p));
    if (match) {
      doprint(" %2d: '%s' = '%s'", ++i, search->first.c_str(), search->second.c_str());
      if (first)
        first = false;
      else
        ret = ret + ",";
      ret = ret + "{\"id\": \"" + search->first + "\", \"val\":\"" + search->second + "\"}"; 

    }
    search++;
  }
  return ret + "]}";
}



String doRam(const char* param, String value) {
  const char *s = param;
  char* s1;
  String ret;
  while (*s && (*s <= ' '))
    s++;
  if ( strchr ( s, '?' ) )
  {
    if ( value != "0" )
      ret = doRamlist(value.c_str());
    else
      ret = doRamlist ( NULL );
  }
  else if ( strchr ( s, '-' ) ) 
  {
    ramdelkey ( value.c_str() );
    ret = "RAM." + value + " is now deleted.";
  }
  else if ( (s1 = strchr ( s, '.' )) )
  {
    String str = String ( s1 + 1 ) ;
    chomp_nvs ( str ) ;
    if ( str.length() > 0 ) {
      ramsetstr ( str.c_str(), value);
      ret = "RAM." + str + " is now: " + value;
    }
  }
  return ret;
}

//**************************************************************************************************
//                                      D O N V S L I S T                                          *
//**************************************************************************************************
// - command "nvslist": list the full NVS content to Serial (even if DEBUG = 0)                    *
// - if parameter p != NULL, only show entries which have *p as substring in keyname               *
//**************************************************************************************************
void doNvslist(const char* p) {
nvs_stats_t nvs_stats;

  fillkeylist( keynames, namespace_ID );
  int i = 0;
  int idx = 0;
  doprint("NVS content");
  for(int i = 0; i < keynames.size();i++)
//  while (nvskeys[i][0] != '\0') {
    {
    const char *key = keynames[i];
    bool match = (p == NULL);
    if (!match)
      match =  strstr(key, p) != NULL;
    if (match)
      doprint(" %3d: '%s' = '%s'", ++idx, key, nvsgetstr(key).c_str());
  }
  nvs_get_stats(NULL, &nvs_stats);
  doprint ("NVS-Count: UsedEntries = (%d), FreeEntries = (%d), AllEntries = (%d)",
              nvs_stats.used_entries, nvs_stats.free_entries, nvs_stats.total_entries);  
  if ( nvs_stats.free_entries < 2 )  
    doprint ("WARNING: NVS is exhausted. You probably need a bigger partition!");
  else if ( nvs_stats.free_entries < 10 )
    doprint ("Warning: NVS space seems to run low!");
}

void doNvs(const char* param, String value) {
 const char *s = param;
  char* s1;
  if ( strchr ( s, '?' ) )
  {
    if ( value != "0" )
      doNvslist(value.c_str());
    else
      doNvslist ( NULL );
  }
  else if ( strchr ( s, '-' ) ) 
  {
    nvsdelkey ( value.c_str() );
  }
  else if ( (s1 = strpbrk ( s, ".&" )) )
  {
    String str = String ( s1 + 1 ) ;
    chomp_nvs ( str ) ;
    if ( str.length() > 0 )
      nvssetstr ( str.c_str(), value);
  }
}


void doChannels(String value) {
  channelList.clear();
  currentChannel = 0;
  value.trim();
  if (value == "0")
    value = "";
  if (value.length() > 0)
    readDataList(channelList, value);
  numChannels = channelList.size();
  dbgprint("CHANNELS (total %d): %s",  numChannels, value.c_str());
}

void doChannel(String value, int ivalue) {
  if ((ivalue == -1) && (currentChannel >= 0))                // request to stop channel mode?
  {
    currentChannel = -1 - currentChannel;                     // invalidate channel but remember last set...
  }
  else if ((genreId > 0) && (genrePresets > 1))               // Are we in genre playmode?
  {
    int32_t curPreset, newPreset;
    curPreset  = newPreset = genrePreset;
    if (value == "up")  
    {
      ++newPreset;
//      if (++newPreset >= genrePresets)
//        newPreset -= genrePresets;
    }
    else if (value == "down")
    {
      --newPreset;
//      if (--newPreset < 0)
//        newPreset += genrePresets;
    }
    else if (value == "any")
      while (newPreset == curPreset)
        newPreset = random(genrePresets);
    else if ((ivalue > 0) && (ivalue <= numChannels) && (ivalue != currentChannel))   // could be channel = number
    {
      if (currentChannel < 0)
      {
        currentChannel = -1-currentChannel;
        if (ivalue == currentChannel)
          --curPreset;
      }
      if (curPreset == newPreset) 
      {
        int chanDistance = genrePresets / numChannels;
        if ( 0 == currentChannel )  
          currentChannel = 1;
        if (0 == chanDistance)
          chanDistance = 1;
//      newPreset = curPreset - (currentChannel - 1) * chanDistance;
//      newPreset = newPreset + (ivalue - 1) * chanDistance ;
        newPreset = curPreset - (genrePresets * (currentChannel - 1)) / numChannels ;
        newPreset = newPreset + (genrePresets * (ivalue - 1)) / numChannels;
/*
      if (newPreset >= genrePresets)
//        while (newPreset >= genrePresets)
//          newPreset -= genrePresets;
        newPreset = newPreset - genrePresets * (newPreset / genrePresets) ;
      else if (newPreset < 0)
          newPreset = newPreset + genrePresets * (1 + (-newPreset - 1) / genrePresets) ;
//        while (newPreset < 0)
//          newPreset += genrePresets;
*/
      /*
      int chanDelta = ivalue - currentChannel;
      int chanDistance = genrePresets / gchannels;
      if (chanDelta < 0)
        chanDelta = chanDelta + gchannels;
      if (0 == chanDistance)
        chanDistance = 1;
      newPreset = (curPreset + chanDelta * chanDistance) % genrePresets ;
      */
      }
      currentChannel = ivalue;
    }
    if (newPreset != curPreset)
    {
      char cmd[30];
      if (currentChannel < 0)
        currentChannel = -1 - currentChannel;
      if (gverbose)
        doprint("Genre preset changed by channel command '%s' from %d to %d", value.c_str(), curPreset, newPreset) ;
      sprintf(cmd, "gpreset=%d", newPreset);
      //analyzeCmd( cmd );
      cmd_backlog.push_back(String(cmd));
    }    
  }
  else if (channelList.size() > 0) {
    int16_t channel = currentChannel;
    //    Serial.printf("CurrentChannel = %d, value=%s, ivalue=%d\r\n", channel, value.c_str(), ivalue);
    if (channel)
      if (channelList[channel - 1] != ini_block.newpreset) {
        //        Serial.println("Channel set to ZERO: does not match current preset");
        channel = currentChannel = 0;
      }
    if (ivalue) {
      if (ivalue <= channelList.size()) {
        channel = ivalue;
      }
    } else {
      if (!channel) {
        currentChannel = channelList.size();
        while ((currentChannel > 0) && (ini_block.newpreset != channelList[currentChannel - 1]))
          currentChannel--;
        channel = currentChannel;
      }
      if (value == "up") {
        if (channel) {
          if (channel < channelList.size())
            channel++;
        } else
          channel = 1;
      } else if (value == "down") {
        if (channel) {
          if (channel > 1)
            channel--;
        } else
          channel = channelList.size();
      } else if (value = "any") {
        if (0 == channel)
          channel = 1 + random(channelList.size());
        else if (channelList.size() > 1)
          while (channel == currentChannel)
            channel = 1 + random(channelList.size());
      }
    }
    if (channel != currentChannel) {
      //      Serial.printf("New Channel found: %d (was %d) with preset=%d\r\n", channel, currentChannel, channelList[channel-1]);
      char s[20];
      sprintf(s, "%d", channelList[channel - 1]);
      analyzeCmd("preset", s);
      currentChannel = channel;
    }
  }
}

void doInlist(const char *p) {
  RetroRadioInput::showAll(p);
}




char *retroRadioLoops[12];
int numLoops = 0;

void readprogbuttonsRetroRadio()
{
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
          char s[200];
          sprintf(s, "in.gpio_%02d=src=d%d,mode=2,on0=gpio_%02d,start", pinnr, pinnr, pinnr);
          analyzeCmd(s);
          progpin[i].reserved = true;                       // reserve it so original readprogbuttons
          // does not handle it
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
          char s[200];
          sprintf(s, "in._touch_%02d_=src=t%d,mode=1,on0=touch_%02d,start", i, i, i);
          analyzeCmd(s);
          dbgprint ( "touch_%02d will execute %s",          // Show result
                     i, val.c_str() ) ;
          touchpin[i].reserved = true;                      // reserve it so original readprogbuttons
          // does not handle it
        }
        else
        {
          dbgprint ( "touch_%02d pin (GPIO%02d) is reserved for I/O!",
                     i, pinnr ) ;
        }
      }
    }
  }
}

static bool EthernetFound = false;
#if (ETHERNET==1)
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
      ETH.setHostname(RADIONAME);
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
  uint32_t ethTimeout = eth_timeout * 1000;
  if (ethTimeout == 0)
    return false;
/*  
  if (nvssearch("eth_timeout"))                                   // Ethernet timeout in preferences?
  {
    String val = String(nvsgetstr ("eth_timeout"));               // De-reference to another NVS-Entry?
    chomp(val);
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
*/  
  WiFi.disconnect(true);                                          // Make sure old connections are stopped
  myStartTime = millis();
  WiFi.onEvent(ethevent);                                         // Event handler to catch connect status
  if (ETH.begin(eth_phy_addr, eth_phy_power, eth_phy_mdc,
                eth_phy_mdio, (eth_phy_type_t)eth_phy_type,
                (eth_clock_mode_t)eth_clk_mode))
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



#endif


static volatile IR_data  ir = { 0, 0, 0, 0, IR_NONE } ;         // IR data received
sv int8_t ir_pin = -1;
//sv bool IRseen = false;

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
void IRAM_ATTR decoder_RR_IRRC5(uint32_t intval, uint32_t t0, int pin_state)
{
  //#define RC5SHORT_MIN 444   /* 444 microseconds */
  //#define RC5SHORT_MAX 1333  /* 1333 microseconds */
  //#define RC5LONG_MIN 1334   /* 1334 microseconds */
  //#define RC5LONG_MAX 2222   /* 2222 microseconds */
  //    IRseen = true;
  const uint8_t trans[4] = {0x01, 0x91, 0x9b, 0xfb} ; // state transition table
  sv uint8_t rc5_counter = 14 ;                       // Bit counter (counting downwards)
  sv uint16_t rc5_command = 0 ;                       // The command received
  sv RC5State rc5_state = RC5STATE_BEGIN ;            // State of the RC5 decoder statemachine
  sv uint32_t rc5_lasttime = 0 ;                      // last time a valid command has been received
  /* VS1838B pulls the data line up, giving active low,
     so the output is inverted. If data pin is high then the edge
     was falling and vice versa.

      Event numbers:
      0 - short space
      2 - short pulse
      4 - long space
      6 - long pulse
  */

  uint8_t event = pin_state ? 2 : 0 ;
  if (intval > 1333 && intval <= 2222) // "Long" event detected?
  {
    event += 4 ;
  }
  else if (intval < 444 || intval > 2222)
  {
    /* If delay wasn't long and isn't short then
       it is erroneous so we need to reset but
       we don't return from interrupt so we don't
       loose the edge currently detected. */
    rc5_counter = 14 ;                   // Bit counter (counting downwards)
    rc5_command = 0 ;                    // The command received
    rc5_state = RC5STATE_BEGIN ;         // State of the RC5 decoder statemachine
  }
  if (rc5_state == RC5STATE_BEGIN)        // First bit?
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
         error so reset. */
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
        Mid0 is ok, but if we finish in MID1 we need to wait
        for START1 so the last edge is consumed. */
      if (rc5_counter == 0 && (rc5_state == RC5STATE_START1 || rc5_state == RC5STATE_MID0))
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
void IRAM_ATTR decoder_RR_IRNEC ( uint32_t intval, uint32_t t0 )
{
  //  IRseen = true;
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
      { // initial key or previous repeat
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
void IRAM_ATTR isr_RR_IR()
{
  sv uint32_t      t0 = 0 ;                          // To get the interval
  uint32_t         t1, intval ;                      // Current time and interval since last change
  int pin_state = digitalRead(ir_pin);     // Get current state of InputPin

  t1 = micros() ;                                    // Get current time
  intval = t1 - t0 ;                                 // Compute interval
  t0 = t1 ;                                          // Save for next compare

  decoder_RR_IRRC5 ( intval, t0, pin_state ) ;          // Run decoder for Philips RC5 protocol

  decoder_RR_IRNEC ( intval, t0 ) ;                     // Run decoder for NEC protocol
}

//**************************************************************************************************
//                                          I S R _ I R N E C                                      *
//**************************************************************************************************
// Interrupts received from VS1838B on every change of the signal.                                 *
// This routine will call the decoder for NEC protocol only.                                       *
//**************************************************************************************************
void IRAM_ATTR isr_RR_IRNEC()
{
  sv uint32_t      t0 = 0 ;                          // To get the interval
  uint32_t         t1, intval ;                      // Current time and interval since last change
  //int pin_state = digitalRead(ir_pin);               // Get current state of InputPin

  t1 = micros() ;                                    // Get current time
  intval = t1 - t0 ;                                 // Compute interval
  t0 = t1 ;                                          // Save for next compare


  decoder_RR_IRNEC ( intval, t0 ) ;                     // Run decoder for NEC protocol
}


//**************************************************************************************************
//                                          I S R _ I R R C 5                                      *
//**************************************************************************************************
// Interrupts received from VS1838B on every change of the signal.                                 *
// This routine will call the decoder for RC5 protocol only.                                       *
//**************************************************************************************************
void IRAM_ATTR isr_RR_IRRC5()
{
  sv uint32_t      t0 = 0 ;                          // To get the interval
  uint32_t         t1, intval ;                      // Current time and interval since last change
  int pin_state = digitalRead(ir_pin);               // Get current state of InputPin

  t1 = micros() ;                                    // Get current time
  intval = t1 - t0 ;                                 // Compute interval
  t0 = t1 ;                                          // Save for next compare

  decoder_RR_IRRC5 ( intval, t0, pin_state ) ;          // Run decoder for Philips RC5 protocol

}


void setupIR() {
  String val;
  ir_pin = ini_block.ir_pin;
  if ( ir_pin >= 0 )
  {
    dbgprint ( "Enable pin %d for IR (NEC)",
               ir_pin ) ;
    pinMode ( ir_pin, INPUT ) ;                // Pin for IR receiver VS1838B
    attachInterrupt ( ir_pin,                  // Interrupts will be handle by isr_IR
                      isr_RR_IRNEC, CHANGE ) ;
  }
  if ( ir_pin < 0)
  {
    val = nvsgetstr("pin_ir_necrc5");
    if (val.length() > 0)                                 // Parameter in preference?
    {
      ir_pin = val.toInt() ;                            // Convert value to integer pinnumber
    }
    if (ir_pin >= 0)
    {
      dbgprint ( "Enable pin %d for IR (NEC + RC5)", ir_pin);
      pinMode ( ir_pin, INPUT ) ;                        // Pin for IR receiver VS1838B
      attachInterrupt ( ir_pin,                  // Interrupts will be handle by isr_IR
                        isr_RR_IR, CHANGE ) ;

    }
  }
  if ( ir_pin < 0)
  {
    val = nvsgetstr("pin_ir_rc5");
    if (val.length() > 0)                                 // Parameter in preference?
    {
      ir_pin = val.toInt() ;                            // Convert value to integer pinnumber
    }
    if (ir_pin >= 0)
    {
      dbgprint ( "Enable pin %d for IR (RC5)", ir_pin);
      pinMode ( ir_pin, INPUT ) ;                        // Pin for IR receiver VS1838B
      attachInterrupt ( ir_pin,                  // Interrupts will be handle by isr_IR
                        isr_RR_IRRC5, CHANGE ) ;

    }
  }

  ini_block.ir_pin = -1;
  if (ir_pin >= 0)
    reservepin(ir_pin);
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

bool execute_ir_command ( String type, bool verbose = true )
{
  const char *protocol_names[] = {"", " (NEC)", " (RC5)"} ;     // Known IR protocol names
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
      { // If not
        sprintf ( mykey, "ir_%04Xr", ir.command );              // Search for "unspecific" longpress event ir_XXXXr
        ret = execute_ir_command ( String(mykey), false );
      }
    }
  }
  if (! done )                                                  // Did we already (tried) some key(s) (can be from longpress only)
  { // If not, mykey now contains the NVS key to check
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
      chomp ( val );
      dbgprint ( "IR code for %s%sreceived. Will execute %s",
                 mykey, protocol_names[ir.protocol], val.c_str() ) ;
      reply = analyzeCmdsRR ( val ) ;                         // Analyze command and handle it
      dbgprint ( reply ) ;                                  // Result for debugging
    }
  }
  return ret;
}



//**************************************************************************************************
//                                     S C A N I R R R                                             *
//**************************************************************************************************
// See if IR input is available.  Execute the programmed command(s).                               *
//**************************************************************************************************
void scanIRRR()
{
  //  char        mykey[20] ;                                   // For numerated key
  //  String      val ;                                         // Contents of preference entry
  //  const char* reply ;                                       // Result of analyzeCmd
  //  uint16_t    code ;                                        // The code of the Remote key
  //  uint16_t    repeat_count ;                                // Will be increased by every repeated
  // call to scanIR() until the key is released.
  // Zero on first detection

  if ( ir_pin < 0)
    return;

  if ( ir.exitcommand != 0 )                                // Key release detected? By ISR! exitcommand contains info on previous key
  {                                                         // ir.command/ir.protocol contains the new key info
    execute_ir_command ( "x" ) ;                            // 
    ir.exitcommand = 0;
    ir.exittimeout = 0;
    doprint("IREXIT by new keypress: %04X", ir.command);
  }
  if ( ir.protocol != IR_NONE )                             // Any input?
  {
    doprint("IR Input, protocol: %d, key: %04X", ir.protocol, ir.command);
    //    char *protocol_names[] = {"", "NEC", "RC5"} ;           // Known IR protocol names
    //    code = ir.command ;                                     // Key code is stored in lower word of ir_value
    //    repeat_count = ir_value >> 16 ;                         // Repeat counter in upper word of if_value
    if ( 0 < ir.repeat_counter )                            // Is it a "longpress"?
    { // In case of a "longpress", we will first search
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
        doprint("IR Input exit, protocol: %d, key: %04X, last: %ld", ir.protocol, ir.command, millis() - ir.exittimeout);
        ir.exitcommand = ir.command ;                       // Set exitcode
        ir.command = 0 ;                                    // Clear code info
        ir.repeat_counter = 0 ;                             // Clear repeat counter (just to be sure)
        ir.exittimeout = 0 ;                                // Clear exitcode timeout (jsut to be sure)
        execute_ir_command ( "x" );                         // Search NVS for "key release" ir_XXXXx
        ir.exitcommand = 0 ;                                // Clear exitcode after it has been used.
        ir.protocol = IR_NONE;                              // Code has been handled.
        doprint("IREXIT by timeout.");
      }
  }
}



void setupRR(uint8_t setupLevel) {
  if (setupLevel == SETUP_START)
  {
    touch_pad_init();
    touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
    touch_pad_filter_start(16);
    //adc_power_on();
    readprogbuttonsRetroRadio();
    setupReadFavorites ();
    setupIR();
    if (nvssearch("pin_reset"))
    {
      int resetPin = nvsgetstr("pin_reset").toInt();
      if (resetPin >= 0)
      {
        digitalWrite(resetPin, HIGH);
        pinMode(resetPin, OUTPUT);
        dbgprint("VS-Reset pin is: %d.", resetPin);
      }
    }
//  }
//  else if (setupLevel == SETUP_NET)
//  {
#if (ETHERNET==1)
    //adc_power_on();                                     // Workaround to allow GPIO36/39 as IR-Input
    if (bootmode != "ap")
    {
      NetworkFound = connecteth();                        // Try ethernet
      if (NetworkFound)                                   // Ethernet success??
        WiFi.mode(WIFI_OFF);
    }
#endif
  }
  else if (setupLevel == SETUP_DONE)
  {
#if defined(RESET_ON_WIFI_FAIL)
  if (!NetworkFound)
    ESP.restart();
#endif

   if (NetworkFound && !EthernetFound){
      uint32_t wifiChannel = WiFi.channel();
      dbgprint("WiFi Channel is: %d", wifiChannel);
      dbgprint("WiFi MAC is: %s", WiFi.macAddress().c_str());
      if (ESP_OK == esp_now_init())
      {
        //uint8_t peerMac[] = {0xdc, 0x4f, 0x22, 0x17, 0xf8, 0xd9};
        //uint8_t peerMac[] = {0x3C, 0x61, 0x05, 0x0B, 0xE3, 0x80};

        dbgprint("ESP-Now init success!!") ;
        ini_block.espnowmode = ini_block.espnowmodetarget ;
/*        
        esp_now_peer_info_t peerInfo;
        memset((void *)&peerInfo, 0, sizeof(peerInfo));
        memcpy(peerInfo.peer_addr, peerMac, 6);
        peerInfo.channel = 9;  
        peerInfo.encrypt = false;
        if (esp_now_add_peer(&peerInfo) != ESP_OK)
          dbgprint("Error adding ESP-Now peer!");
*/
        esp_now_register_recv_cb([](const unsigned char *mac, const unsigned char *data, int len)
          {

          struct ESPNowData {
            ESPNowData(const unsigned char *Mac, const unsigned char *Data, int Len) {
              memcpy(&mac, Mac, 6);
              data = NULL;
              if (0 != (len = Len))
                if (NULL != (data = (unsigned char*)malloc(Len)))
                  memcpy(data, Data, len);
              rcvTime = millis();
            };
            ~ESPNowData() {
              if (data)
                free(data);
            }
            unsigned char mac[6];
            unsigned char* data;
            int len;
            uint32_t rcvTime;
          };

          static std::vector<ESPNowData *> espNowDebounceBuffer;

          int idx = 0;
          uint8_t prefixLen = 0;
          uint8_t plen;
          ESPNowData *p;
          plen = (uint8_t)data[0];
          /*
          Serial.printf("Got Something from {0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x}. PLEN=%d (len=%d) ", 
          (uint8_t)mac[0], (uint8_t)mac[1], (uint8_t)mac[2], (uint8_t)mac[3], (uint8_t)mac[4], (uint8_t)mac[5],
          plen, len);
          */
          if ((plen >= 2) && (plen < len))
          {
            plen--;
            if ((plen == 5) && (0 != (ini_block.espnowmode & 2)))
            {
              if (0 == memcmp(data+1, "RADIO", 5))
                prefixLen = 7;
            }
            else
              if ((plen == strlen(RADIONAME)) && (0 != (ini_block.espnowmode & 1)))
                if (0 == memcmp(data + 1, RADIONAME, plen))
                  prefixLen = plen + 2;
          }
          if (0 == prefixLen)
          {
            //Serial.println("Invalid Data!");
            return;
          }
          else
          {

            while (idx < espNowDebounceBuffer.size()) {
              p = espNowDebounceBuffer[idx];
              if ((millis() - p->rcvTime) > 500) {
                delete(p);
                espNowDebounceBuffer.erase(espNowDebounceBuffer.begin());
              }
              else
              {
                if (len == p->len)
                  if (0 == memcmp(mac, p->mac, 6))
                    if (0 == memcmp(data, p->data, len))
                    {
                      break;
                    }
                idx++;
              }
            }

            if (idx < espNowDebounceBuffer.size())
              ;//Serial.printf("This is a duplicate");
            else
            {
              p = new ESPNowData(mac, data, len);
              len = len - prefixLen;
              if (len > 0)
              {
                char *s = (char*)malloc(len + 7);
                if (s)
                {
                  memcpy(s, mac, 6);
                  memcpy(s + 6, data + prefixLen, len);
                  s[len + 6] = 0;
//                  memcpy(s + len - 1, mac, 6);
                  //Serial.printf(" [added to ESPNow-Backlog: %s] ", s + 6);
                  
                  espnowBacklog.push_back(s);


                }
              }
              espNowDebounceBuffer.push_back(p);

            }  
            //Serial.printf("! (BufLen=%d)\r\n\n", espNowDebounceBuffer.size());
            
            //Serial.flush();


            }
          }
          );
      }
    }

    char s[20] = "::setup";
    int l = strlen(s);
    setupDone = true;
    doprint("Try to open Genres on LITTLEFS!");
    if (genres.begin());
    {
      doprint("LITTLEFS open for genres was a success!");
    }
    analyzeCmdsRR("volume=&volume;preset=&preset;toneha=&toneha;tonela=&toneloa;tonehf=&tonehf;"
                  "tonelf=&tonelf;station=&station");
    doprint("Running commands from NVS '%s'", s);
    analyzeCmdsRR ( nvsgetstr(s) );
    s[l + 1] = 0;
    for (int i = 0; i < 10; i++)
    {
      s[l] = '0' + i;
      dbgprint("Running commands from NVS '%s'", s);
      analyzeCmdsRR ( nvsgetstr(s) );
    }
    strcpy(s, "::loop");
    l = strlen(s);
    String loopcmd = nvsgetstr(s);
    numLoops = 0;
    int i = 0;
    s[l + 1] = 0;
    do
    {
      loopcmd.trim();
      if (loopcmd.length() > 0)
      {
        retroRadioLoops[numLoops] = strdup(loopcmd.c_str());
        dbgprint("Found loop command: %s", retroRadioLoops[numLoops]);
        numLoops++;
      }
      if (i < 10)
      {
        s[l] = '0' + i;
        loopcmd = nvsgetstr(s);
      }
      i++;
    } while (i < 11);
    retroRadioLoops[numLoops] = NULL;
#if defined(BLUETOOTH)
    if (-1 == ini_block.bt_off)
      if (0 <= ini_block.bt_pin)
      {
        String s = String("in.**bt**=src=d") + ini_block.bt_pin + ",on0={reset=bt},start";
        analyzeCmd(s.c_str());
      }
#endif
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    doprint("This is ESP32 chip with %d CPU cores, WiFi%s%s, ",
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    doprint("silicon revision %d, ", chip_info.revision);

    doprint("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
  
  }
}


void favplayinfoloop();


void loopRR() {
  int i;
  scanIRRR();
  RetroRadioInput::checkAll();
  if (espnowBacklog.size() > 0)
  {
    char *s0 = espnowBacklog[0];
    //char *mac = s + strlen(s) + 1;
    dbgprint("Command from ESP-Now[%02X.%02X.%02X.%02X.%02X.%02X]: %s", 
        s0[0], s0[1], s0[2], s0[3], s0[4], s0[5], s0 + 6);
    analyzeCmdsRR(String(s0 + 6));

    espnowBacklog.erase(espnowBacklog.begin());
    free(s0);
  }

  if (numLoops) {
    static int step = 0;
    int deb = DEBUG;
    //bool info = (currentpreset == ini_block.newpreset) && currentpreset >= 0;
    DEBUG = 0;//info?1:0;
    //if (info) dbgprint("\r\nBEFORE ::loop[%d] currentpreset=%d, ini_block.newpreset=%d", step, currentpreset, ini_block.newpreset);
    analyzeCmdsRR(String(retroRadioLoops[step++]));
    //if (info) dbgprint("\r\nAFTER ::loop[] currentpreset=%d, ini_block.newpreset=%d", currentpreset, ini_block.newpreset);
    DEBUG = deb;
    if (step >= numLoops)
      step = 0;
    /*
    for (int i = 0; i < numLoops; i++) 
    {
      DEBUG = 0;
      analyzeCmdsRR(String(retroRadioLoops[i]));
      yield();
    }
    */
    DEBUG = deb;
  }
  

  for (i = 0; i < cmd_backlog.size();i++)
  {
    dbgprint(analyzeCmdsRR(cmd_backlog[i]));
    //cmd_backlog.erase(cmd_backlog.begin());
  }
  if (i > 0)
    cmd_backlog.clear();
  genres.loop();
  //favplayinfoloop();
  if (mqttrcv_backlog.size() > 0)
  {
    const char *topic = mqttrcv_backlog[0];
    const char *payload = topic + strlen(topic) + 1;
    //dbgprint("\r\nCheck MQTT-Fire: %s->%s", topic, payload);
    RetroRadioInputReaderMqtt::fire(topic, payload);
    mqttrcv_backlog.erase(mqttrcv_backlog.begin());
    free((void *)topic);
  } 

}


void doCall ( String param, String value, bool doShow ) {
//bool returnFlag = false;
static int calllevel = 0;
//bool doShow = (param.c_str()[4] == 'v') || (param.c_str()[4] == 'V');
    if (param.length() > (doShow?5:4))
    {
      param = param.substring( doShow?5:4 );
      chomp ( param ) ;
      const char *s = param.c_str();
      if ( *s == '.' )
      {
        const char *p;
        s++;
        if ( NULL == (p = strchr(s, '(')) )
          p = s + strlen(s);
        char buf[p - s + 1];
        strncpy(buf, s, p - s + 1);
        buf[p - s] = 0;
        String dummy = String(buf);
        chomp ( dummy );
        if ( dummy.length() > 0 )
          value = dummy;
        param = String ( p );
      }
      if ( param.c_str()[0] == '(') 
      {
        String group, dummy;
        parseGroup (param, group, dummy);
        param = group;
      } else
        param = "";
    } else
      param = "";
    if (doShow) doprint ( "call(%s)=%s", param.c_str(), value.c_str());  
    value = ramsearch(value.c_str())?ramgetstr(value.c_str()):nvsgetstr(value.c_str()) ;
    value.trim();
    if (value.length() > 0)
    {
      substitute(value, param.c_str());
      if (doShow) doprint ( "call sequence (after substitution): %s", value.c_str());
      doprint("callLevel: %d", ++calllevel);
      analyzeCmdsRR ( value );  
    }
    else  
      if (doShow)
        doprint("No entry found!");
    calllevel--;
}

void doCmd ( String param, String value ) {
//bool returnFlag = false;
static int calllevel = 0;
bool doShow = (param.c_str()[3] == 'v') || (param.c_str()[3] == 'V');
    if (param.length() > (doShow?4:3))
    {
      param = param.substring( doShow?5:4 );
      chomp ( param ) ;
      const char *s = param.c_str();
      if ( *s == '.' )
      {
        const char *p;
        s++;
        if ( NULL == (p = strchr(s, '(')) )
          p = s + strlen(s);
        char buf[p - s + 1];
        strncpy(buf, s, p - s + 1);
        buf[p - s] = 0;
        String dummy = String(buf);
        chomp ( dummy );
        if ( dummy.length() > 0 )
          value = dummy;
        param = String ( p );
      }
      if ( param.c_str()[0] == '(') 
      {
        String group, dummy;
        parseGroup (param, group, dummy);
        param = group;
      } else
        param = "";
    } else
      param = "";
    if (doShow) doprint ( "cmd(%s)=%s", param.c_str(), value.c_str());  
    //value = ramsearch(value.c_str())?ramgetstr(value.c_str()):nvsgetstr(value.c_str()) ;
    value.trim();
    if (value.length() > 0)
    {
      substitute(value, param.c_str());
      if (doShow) doprint ( "cmd sequence (after substitution): %s", value.c_str());
      doprint("callLevel: %d", ++calllevel);
      analyzeCmdsRR ( value );
      ramsetstr("__cmdreply", cmdReply);  
    }
    else  
    {
      if (doShow)
        doprint("Commands to execute");
      ramsetstr("__cmdreply", "");
    }
    calllevel--;
}

extern uint8_t FindNsID ( const char* ns );


void domvcplsprefsfrom(String name, char mode) {
  nvs_handle cp_handle;
  uint8_t namespaceid;
  name = name.substring(0, NVS_KEY_NAME_MAX_SIZE - 1);
  String myName = String(RADIONAME).substring(0, NVS_KEY_NAME_MAX_SIZE - 1);
  if ((name == myName) && (mode != 'l'))
  {
    dbgprint("Source namespace must be different from current namespace(%s) for this operation.", myName.c_str());
    return;
  }
  namespaceid = FindNsID(name.c_str());
  if (0 == namespaceid)
  {
    dbgprint("Namespace(%s) is not in NVS", name.c_str());
    return;
  }
  dbgprint("Start to %s preference settings from namespace(%s), namespaceID=%d",
    (mode == 'm')?"move":((mode=='c')?"copy":"list"), name.c_str(), namespaceid);
  std::vector<const char*> cpkeys;
  fillkeylist(cpkeys, namespaceid);
  if ( 0 == cpkeys.size())
  {
    dbgprint("Ignored: no preferences defined/no valid preferences in namespace(%s) ", name.c_str());
    return;
  }
  esp_err_t nvserr = nvs_open ( name.c_str(), NVS_READWRITE, &cp_handle ) ;
  if (0 != nvserr)
  {
    dbgprint("Unexpected but fatal: could not open namespace(%s), nvserr=%d", name.c_str(), nvserr);
    return;
  }
  int idx = 0;
  for (int i = 0; i < cpkeys.size(); i++)
  {
      const char *key = cpkeys[i];
      char   nvs_buf[NVSBUFSIZE] ;              // Buffer for contents
      size_t        len = NVSBUFSIZE ;          // Max length of the string, later real length
      nvs_buf[0] = '\0' ;                       // Return empty string on error
      nvserr = nvs_get_str ( cp_handle, key, nvs_buf, &len ) ;
      if ( nvserr )
        {
        dbgprint ( "nvs_get_str failed %X for key %s, keylen is %d, len is %d!",
               nvserr, key, strlen ( key), len ) ;
        dbgprint ( "Contents: %s", nvs_buf ) ;
        }
      else
        {
          if ((0 == idx) && (mode != 'l'))
          {
            dbgprint("At least one entry found. Will delete now all current NVS content");
            nvsclear();
          }    
          idx++;
          dbgprint("%3d: %s=%s", idx, key, nvs_buf);
          if (mode != 'l')
          {
            nvserr = nvs_set_str ( nvshandle, key, nvs_buf ) ; // Store key and value
            if ( nvserr )                                          // Check error
            {
              dbgprint ( "nvssetstr failed!" ) ;
            }
            if (mode == 'm')
              nvs_erase_key( cp_handle, key ) ;
          }
        }
  }
  if ((idx > 0) && (mode != 'l'))
  {
    resetreq = true;
    if (mode == 'm')
      nvs_erase_all(cp_handle);
    nvs_commit(cp_handle);
    nvs_close(cp_handle);
    nvs_commit(nvshandle);
    nvs_close(nvshandle);
  }
  return;
  /*
  esp_err_t     nvserr = nvs_open ( name.substring(0, NVS_KEY_NAME_MAX_SIZE - 1).c_str(), NVS_READWRITE, &cp_handle ) ;  // No, open nvs
  if (nvserr)
  {
    dbgprint("Error opening NVS namespace(%s)=%d", name.substring(0, NVS_KEY_NAME_MAX_SIZE - 1).c_str(), nvserr);
    return;
  }
  nvs_get_used_entry_count(cp_handle, &cp_entries);
  if (0 == cp_entries)
  {
    dbgprint("There are no preferences defined in namespace(%s)", name.substring(0, NVS_KEY_NAME_MAX_SIZE - 1).c_str());
    nvs_close(cp_handle);
    return;
  }
  dbgprint("%s %d entries from namespace(%s)", 
      isMove?"Moving":"Copying", cp_entries, name.substring(0, NVS_KEY_NAME_MAX_SIZE - 1).c_str());
  nvs_iterator it = nvs_entry_find("name", name.substring(0, NVS_KEY_NAME_MAX_SIZE - 1).c_str(), NVS_TYPE_ANY);
  while (it != NULL) {
          nvs_entry_info info;
          nvs_entry_info(it, &info);
          it = nvs_entry_next(it);
          dbgprint("key '%s', type '%d' \n", info.key, info.type);
  };
  if (isMove)
    nvs_commit(cp_handle);
  nvs_close(cp_handle);
  */
}

//extern int connectDelay;

void getTTSUrl(const char* cmd, String& value, int len) {
  if (value.length() > 0)
    if (cmd[len] == '.')
      if (cmd[len + 1] == 't')
      {
        String ret;
        String lang = String(cmd + len + 2);
        if (!announceMode)
          connectDelayBefore = connectDelay;
        connectDelay = 8;
        if (lang.length() == 0)
          lang = "en";
        const char *v = value.c_str();
        int l = strlen(v);
        String encodedString = "";
        char c;
        char code0;
        char code1;
//        char code2;
        for (int i = 0; i < l; i++)
        {
          c = v[i];
          if (c == ' ')
          {
            ret += '+';
          }
          else if (isalnum(c))
          {
            ret += c;
          }
          else if ('=' == c)
          {
            ret += '+';
            i++;
          }
          else if ('#' == c)
          {
            break;
          }
          else
          {
            code1 = (c & 0xf) + '0';
            if ((c & 0xf) > 9)
            {
              code1 = (c & 0xf) - 10 + 'A';
            }
            c = (c >> 4) & 0xf;
            code0 = c + '0';
            if (c > 9)
            {
              code0 = c - 10 + 'A';
            }
            //code2 = '\0';
            ret += '%';
            ret += code0;
            ret += code1;
            //encodedString+=code2;
          }
        }        
/*        
        for (int i = 0; i < l;i++)
          if (v[i] == ' ')
            ret = ret + "%20";
          else if (v[i] == '=')
          {
            ret = ret + " " + String(v + i + 1);
            break;
          }
          else
            ret = ret + v[i];
*/
        value = String("translate.google.com/translate_tts?ie=UTF-8&tl=") + lang + String("&client=tw-ob&q=") + ret;
      }
}

String chomp_nvs_ram_sys(const char *keyWithPrefix) {
  char firstChar = *keyWithPrefix;
  const char* key = keyWithPrefix + 1;
  String ret = "";
  char nextChar = *key;

  if (isalnum(nextChar) || ('_' == nextChar) || ('$' == nextChar))
  {
    bool replaced = false;
    if (('@' == firstChar) || ('.' == firstChar))
    {
      replaced = ramsearch(key);
      if (replaced)
        ret =ramgetstr(key);
    }
    if (!replaced)
    {
      if (('@' == firstChar) || ('&' == firstChar))
      {
        replaced = nvssearch(key);
        if (replaced)
          ret =nvsgetstr(key);
      }
      if (!replaced)
      {
        if ('%' == firstChar)
        {
          replaced = nvssearch(key);
          if (replaced)
            ret = nvsgetstr(key, false);
        }
        else if ('~' == firstChar)
        {
          replaced = true;
          ret = readSysVariable(key);
        }
      }
    }
  }
  else
    ret = String(keyWithPrefix);
  ret.trim();
  return ret;
}


void chompValue (String& value) {
  value.trim();
  const char* valuec_str = value.c_str();
  if (value.length() > 0) {
    char firstChar = value.c_str()[0];
    if ('(' == firstChar)
    {
      String group, dummy;
      parseGroup ( value, group, dummy );
      value = group;
    } 
    else if (('"' != firstChar) && ('\'' != firstChar))
    {
      dbgprint("Check first char of value: %c", firstChar);
      if (strchr("@&.~%", firstChar))
      {
        dbgprint("Run chomp_all for: %s", valuec_str);

        value = chomp_nvs_ram_sys(valuec_str);
      }
    }
    else
    {
      uint8_t count = 0;
      int haveDouble = 0;
      while ( '\'' == valuec_str[count])
        count++;
      valuec_str = valuec_str + (count?count:1) ;
      //chomp_nvs(value);
      if (0 != valuec_str[0])
      {
        char valueCopyLen = strlen(valuec_str) + 50;

        char* valueCopy = (char *)malloc(valueCopyLen + 1);
        strcpy(valueCopy, valuec_str);

        bool haveReplaced;
        do
        {
          //char *searchStart = valueCopy;
          char *searchEnd = valueCopy - strlen(valueCopy) - 1;
          haveReplaced = false;
          // 
          char* identBegin;char *identEnd = identBegin = NULL;
          //while (searchStart)
          while (searchEnd >= valueCopy)
          {
            char c = *searchEnd;
            if (NULL != strchr("@&.~%", c))
            {
              if (searchEnd > valueCopy)
                if (*(searchEnd - 1) == c)
                {
                  haveDouble = true;
                  searchEnd--;
                  c = 0;
                }
              if ((c != 0) && (identBegin != NULL))
              {
                int identlen = identBegin - identBegin + 1;
                char id[identlen + 1];
                memcpy(id + 1, identBegin, identlen);
                id[identlen + 1] = 0;
                id[0] = c;
                String replacement = chomp_nvs_ram_sys(id);
                int replacelen = replacement.length();
                int newLen = strlen(valueCopy) + replacelen - identlen;
                if (newLen > valueCopyLen)
                {
                  valueCopyLen = newLen + 50;
                  char *newValueCopy = (char *)malloc(valueCopyLen);
                  strcpy(newValueCopy, valueCopy);
                  identBegin = newValueCopy + (identBegin - valueCopy);
                  identEnd = newValueCopy + (identEnd - valueCopy);
                  searchEnd = newValueCopy + (searchEnd - valueCopy);
                  free(valueCopy);
                  valueCopy = newValueCopy;
                }
                memmove(searchEnd + replacelen, 
                            searchEnd + identlen + 1, 
                            strlen(searchEnd + identlen));
                memcpy(searchEnd, replacement.c_str(), replacelen);
              }
              identBegin = identEnd = NULL;
            }
            else if (isalnum(c) || ('$' == c) || ('_' == c))
            {
              identBegin = searchEnd;
              if (NULL == identEnd)
                identEnd = identBegin;
            }
            else
              identBegin = identEnd = NULL;
            searchEnd--;
            /*
            char *nextfound = strpbrk(searchStart, "@&.~%");
            if (!nextfound)
            {
              searchStart = NULL;
            }
            else if (*nextfound == nextfound[1])// && ('@' == *nextfound))
            {

              //memmove(nextfound, nextfound + 1, strlen(nextfound));
              //*nextfound = '#';
              haveDouble++;
              searchStart = nextfound + 2;
            }
            else
            {
              char *endfound = nextfound + 1;
              int identlen;
              while (isalnum(*endfound) || ('$' == *endfound) || ('_' == *endfound))
                endfound++;
              if ((identlen = (endfound - nextfound)) > 1)
              {
                char identifier[identlen + 1];
                haveReplaced = true;
                memcpy(identifier, nextfound, endfound - nextfound);
                identifier[identlen] = 0;
                String replacement(identifier);
                replacement = chomp_nvs_ram_sys(identifier);
                int replacelen = replacement.length();
                int newLen = strlen(valueCopy) + replacelen - identlen;
                if (newLen > valueCopyLen)
                {
                  valueCopyLen = newLen + 50;
                  char *newValueCopy = (char *)malloc(valueCopyLen);
                  strcpy(newValueCopy, valueCopy);
                  nextfound = newValueCopy + (nextfound - valueCopy);
                  endfound = nextfound + identlen;
                  free(valueCopy);
                  valueCopy = newValueCopy;
                }
                memmove(nextfound + replacelen, endfound, strlen(endfound) + 1);
                if (replacelen)
                  memcpy(nextfound, replacement.c_str(), replacelen);
                searchStart = nextfound + replacelen;
              }
              else
                searchStart = nextfound + 1;
            }
          */
          }
        }
        while (haveReplaced && (--count > 0));
        if (haveDouble)
        {
          char *s = strstr(valueCopy, "@@");
          while (s && haveDouble)
          {
            memmove(s+1, s+2, strlen(s +1));
            haveDouble--;
            s = strstr(s+1, "@@");
          }
        }
        if (haveDouble)
        {
          char *s = strstr(valueCopy, "&&");
          while (s && haveDouble)
          {
            memmove(s+1, s+2, strlen(s +1));
            haveDouble--;
            s = strstr(s+1, "&&");
          }
        }
        if (haveDouble)
        {
          char *s = strstr(valueCopy, "..");
          while (s && haveDouble)
          {
            memmove(s+1, s+2, strlen(s +1));
            haveDouble--;
            s = strstr(s+1, "..");
          }
        }
        if (haveDouble)
        {
          char *s = strstr(valueCopy, "~~");
          while (s && haveDouble)
          {
            memmove(s+1, s+2, strlen(s +1));
            haveDouble--;
            s = strstr(s+1, "~~");
          }
        }
        if (haveDouble)
        {
          char *s = strstr(valueCopy, "%%");
          while (s && haveDouble)
          {
            memmove(s+1, s+2, strlen(s +1));
            haveDouble--;
            s = strstr(s+1, "%%");
          }
        }
        value = String(valueCopy);
        free(valueCopy);
        value.trim();
      }
      else
        value = "";
    }
  }
}

const char* analyzeCmdRR(char* reply, String param, String value, bool& returnFlag) {
  const char *s; 
  char firstChar; 
  bool ret = false;
  chomp(param);
  if (0 == param.length())
    return "";
  String paramOriginal = param;
  param.toLowerCase();

  s = paramOriginal.c_str();
  firstChar = *s; 

  chompValue (value);
/*
  chomp ( value ) ;
  if ( ( value.c_str()[0] == '(' ) )
  {
    String group, dummy;
    parseGroup ( value, group, dummy );
    value = group;
  } 
  else if (value.c_str()[0] != '"')
  {
    chomp_nvs(value);
  }
  else if (value.length() > 1)
  {
    //chomp_nvs(value);
    char valueCopyLen = value.length() + 50;

    char* valueCopy = (char *)malloc(valueCopyLen + 1);
    strcpy(valueCopy, value.c_str() + 1);

    bool haveReplaced;
    do
    {
      char *searchStart = valueCopy;
      haveReplaced = false;
      while (searchStart)
      {
        char *nextfound = strpbrk(searchStart, "@&.~");
        if (!nextfound)
        {
          searchStart = NULL;
        }
        else if (*nextfound == nextfound[1])
        {
          memmove(nextfound, nextfound + 1, strlen(nextfound));
          searchStart = nextfound + 1;
        }
        else
        {
          char *endfound = nextfound + 1;
          int identlen;
          while (isalnum(*endfound) || ('$' == *endfound) || ('_' == *endfound))
            endfound++;
          if ((identlen = (endfound - nextfound)) > 1)
          {
            char identifier[identlen + 1];
            haveReplaced = true;
            memcpy(identifier, nextfound, endfound - nextfound);
            identifier[identlen] = 0;
            String replacement(identifier);
            chomp_nvs(replacement);
            int replacelen = replacement.length();
            int newLen = strlen(valueCopy) + replacelen - identlen;
            if (newLen > valueCopyLen)
            {
              valueCopyLen = newLen + 50;
              char *newValueCopy = (char *)malloc(valueCopyLen);
              strcpy(newValueCopy, valueCopy);
              nextfound = newValueCopy + (nextfound - valueCopy);
              endfound = nextfound + identlen;
              free(valueCopy);
              valueCopy = newValueCopy;
            }
            memmove(nextfound + replacelen, endfound, strlen(endfound) + 1);
            if (replacelen)
              memcpy(nextfound, replacement.c_str(), replacelen);
            searchStart = nextfound + replacelen;
          }
          else
            searchStart = nextfound + 1;
        }
      }
    }
    while (haveReplaced);
    value = String(valueCopy);
    free(valueCopy);
  }
*/
  *reply = '\0';
  if (!isalpha(firstChar))
  {
    ret = true;
    if ( firstChar == '.' )
    {
      char * s1 = strpbrk ( s + 1, "?-" );
      if ( s1 ) 
        strcpy(reply, doRam ( s1, value ).c_str());
      else
        strcpy(reply, doRam ( s , value ).c_str());
    }
    else  if ( firstChar == '&' )
    {
      char * s1 = strpbrk ( s + 1, "?-" );
      if ( s1 ) 
        doNvs ( s1, value );
      else
        doNvs ( s , value );    
    }
    else  if ( firstChar == '~' )
    {
      if (value != "0")
        strcpy(reply, doSysList(value.c_str()).c_str());
      else
        strcpy(reply, doSysList(NULL).c_str());
    }
  
  }
  else if ( (ret = ( param.startsWith ( "call" ))) )
  {
    doCall ( paramOriginal, value, param[4] == 'v' );
  }
  else if ( (ret = ( param.startsWith ( "cmd"))) )
  {
    doCmd ( paramOriginal, value );
  }
  else if ( (ret = ( param.startsWith ( "genre" ))) ) 
  {
    strcpy(reply, doGenre ( param, value ).c_str());
  }
  else if ( (ret = (param == "gpreset")) ) 
  {
    strcpy(reply, doGpreset ( value ).c_str());
  }
  else if ( (ret = param.startsWith("if")) )
  {
    doCalcIfWhile(paramOriginal, "if", value, returnFlag);//doIf(value, argument.c_str()[2] == 'v');
  } 
  else if ( (ret = param.startsWith("calc")) )
  {
    doCalcIfWhile(paramOriginal, "calc", value, returnFlag);//doCalc(value, argument.c_str()[4] == 'v');
  } 
//  else if ( (ret = param.startsWith("whilenot")) )
//  {
//    doCalcIfWhile(param, "while", value, returnFlag, true);
//  }
  else if ( (ret = param.startsWith("while")) )
  {
    doCalcIfWhile(paramOriginal, "while", value, returnFlag);
  }
  else if ( (ret = param.startsWith("idx")) )
  {
    doCalcIfWhile(paramOriginal, "idx", value, returnFlag);
  }
  else if ( (ret = param.startsWith("len")) )
  {
    doCalcIfWhile(paramOriginal, "len", value, returnFlag);
  }
  else if ( (ret = (param.startsWith("ram" ))) ) 
  {
    //Serial.println("RAM FROM NEW SOURCE!!!");
    strcpy(reply, doRam ( paramOriginal.c_str() + 3, value ).c_str()) ;
  }
  else if ( (ret = (param.startsWith("in"))) )
  {
    if (param == "in")
    {
      doInlist(value.length() > 0?value.c_str(): NULL);
    }
    else
    {
      const char *s = paramOriginal.c_str() + 2;
      while (*s && (*s != '.'))
        s++;
      if ( (ret = (*s == '.')))
      //Serial.println("RAM FROM NEW SOURCE!!!");
        doInput ( s + 1, value ) ;
      else
        ret = false;
    }
  }
  else if ( (ret = (param.startsWith("nvs" ))) ) 
  {
    //Serial.println("RAM FROM NEW SOURCE!!!");
    doNvs ( paramOriginal.c_str() + 3, value ) ;
  }
  else if ( (ret = ( param == "channels" )) ) 
  {
    doChannels (value);
  } 
  else if ( (ret = ( param == "channel" )) )
  {
    doChannel (value, atoi ( value.c_str() ) );
  }
  else if ( (ret = (param.startsWith("return" ))) ) 
  {
    returnFlag = true ;
    char *s1 = strchr ( param.c_str() , '.' ) ; 
    if ( s1 )
      strcpy(reply, doRam ( s1, value ).c_str()) ;
  }
  else if ((param == "preset") && ((value == "--stop") || (value.toInt() == -1)))
  {
    currentpreset = ini_block.newpreset = -1;
    dbgprint("\r\n\r\nRESET CURRENTPRESET\r\n\r\n");
    ret = true;
  }
  else if ( ret = param.startsWith("gcfg.") )
  {
    doGenreConfig(param.substring(5), value);
  }
  else if (ret = param.startsWith("fav"))
  {
    strcpy(reply, doFavorite(param, value).c_str());
  } 
  else if ( (ret = (param.startsWith("mqttpub")) ))
  {
    String topic;
    String payload;
    if (param.length() > 7) 
    {
      topic = paramOriginal.substring(8);
      topic.trim();
    }
    char* s = (char *)malloc (topic.length() + value.length() + 2);
    if (s)
    {
      strcpy(s, topic.c_str());
      strcpy(s + 1 + topic.length(), value.c_str());
      mqttpub_backlog.push_back(s);
    }
  }
  else if (ret = (param.startsWith("announce")))
  {
    getTTSUrl(param.c_str(), value, strlen("announce"));
    extern uint8_t announceMode;
    dbgprint("'announce' with announceMode==%d", announceMode);
    if (setAnnouncemode(1))
    {
    String info;
      dbgprint("setAnnouncemode(1) SUCCESS!");
      getAnnounceInfo(value, info, 1);
      host = value;
      hostreq = true;  
    }


//    analyzeCmd("station", value.c_str());
  }
  else if (ret = (param.startsWith("alert")))
  {
    getTTSUrl(param.c_str(), value, strlen("alert"));
    if (setAnnouncemode(2)) 
    {
    String info;
      host = value;
      hostreq = true;  
      getAnnounceInfo(value, info, 2);
      dbgprint("Nach Infosplit: %s#%s", value.c_str(), info.c_str());
    }
    //analyzeCmd("station", value.c_str());
  }
  else if (ret = (param.startsWith("alarm")))
  {
    getTTSUrl(param.c_str(), value, strlen("alarm"));
    if (setAnnouncemode(ANNOUNCEMODE_PRIO3)) 
    {
    String info;
      getAnnounceInfo(value, info, 2);
      dbgprint("Nach Infosplit: %s#%s", value.c_str(), info.c_str());

      host = value;
      hostreq = true;  
    }
  }
  else if (ret = (param == "upload"))
  {
    String filePath;
    int idx = value.indexOf(' ');
    if (idx < 0)
      dbgprint("Both URL and 'path to upload file' (separated by space) must be given!");
    else
    {
      filePath = value.substring(idx + 1);
      value = value.substring(0, idx);
      dbgprint("Download from URL='%s' to file='%s'", value.c_str(), filePath.c_str()); 
      uploadfile.begin(filePath);
    }
  }
  else if (ret = (param == "resume"))
  {
    if (announceMode != 0)
    {
      resumeFlag = true;
      analyzeCmd("stop");
      //setAnnouncemode(0);
    }
    
  }
  else if (ret = (param == "arguments"))
  {
    doArguments(value);
  }
  else if (ret = (param == "espnowmode"))
  {
    if (isdigit(value.c_str()[0]))
    {
      int i = value.toInt();
      if ((i >= 0) && (i < 4))
      {
        if (ini_block.espnowmode >= 0)
          ini_block.espnowmode = i;
        else
          ini_block.espnowmodetarget = i;
      }
    }
    dbgprint("ESP-Now mode is: %d", ini_block.espnowmode) ;
  }
  else if (ret = ((param == "mvprefsfrom") || (param == "cpprefsfrom") || (param == "lsprefsfrom")))
  {
    domvcplsprefsfrom(value, param.c_str()[0]);
  }
  else if ((ret = ( param == "lsnamespaces")))
  {
    std::vector<const char*> namespaces;
    fillnslist(namespaces);
//    dbgprint("Found %d namespaces in NVS.", namespaces.size());
    while (namespaces.size())
      {
        const char *s = namespaces[0];
        dbgprint("%3d: %s", *s, s + 1);
        free((void *)s);
        namespaces.erase(namespaces.begin());
      }
  }
  if ( ret ) 
  {
    if ( value.length() )
    {
      dbgprint ( "Command: %s with parameter %s",
               param.c_str(), value.c_str() ) ;
    }
    else
    {
      dbgprint ( "Command: %s (without parameter)",
               param.c_str() ) ;
    }    
    if ( strlen ( reply ) == 0 ) 
      strcpy ( reply, "Command accepted (RetroRadio)" ) ;
  } 
  else 
  {
    //const char *ret = analyzeCmd ( param.c_str(), value.c_str() ) ;  
    strcpy(reply, analyzeCmd ( param.c_str(), value.c_str() )) ;  
    //dbgprint(reply);
    //return reply;
  }
  return reply;
}




//**************************************************************************************************
//                                   A N A L Y Z E C M D R R                                       *
//**************************************************************************************************
// Handling of the various commands from remote webclient, Serial or MQTT.                         *
// Here a sequence of commands is allowed, commands are expected to be seperated by ';'            *
//**************************************************************************************************
const char* analyzeCmdRR ( const char* commands, bool& returnFlag )
{
  
  uint16_t commands_executed = 0 ;               // How many commands have been executed

  *cmdReply = 0;
  if (returnFlag)
    return cmdReply;
  String cmdstr = String( commands );
  chomp(cmdstr);
  char *cmds = NULL;
  //char * cmds = strdup ( commands ) ;
  if (cmdstr.length() > 0)
    cmds = strdup(cmdstr.c_str());
  if ( cmds )
  {
    char *s = cmds;
/*
    char *s = strchr ( cmds, '#' ) ;
    while (s) 
    {
      char *s1 = s + 1;
      if ('#' == *s1)
      {
        memmove(s, s1, strlen(s));
        s = s1;
      }
      else
      {
        *s = 0;
        s = NULL;
      }
    }
    s = cmds;
*/
    if (!setupDone) {
      s = strchr(cmds, ';');
      if (s)
        *s = 0;
      s = cmds;
    }
    while ( ( NULL != s )  && !returnFlag )
    {
      char *p = s;
      char * next = NULL;
      int nesting = 0;
      while (*p && !next) {
        if ((*p == '(') || (*p == '{'))
          nesting++;
        else if (nesting == 0) {
          if (*p == ';' )
            next = p;
        } else if ((*p == ')') || (*p == '}'))
          nesting--;
        p++;
      }
      if ( next )
      {
        *next = 0 ;
        next++ ;
      }
      //analyzeCmd ( s ) ;
      nesting = 0;
      char *value = (char *)s;
      //dbgprint("looking for parameter for command %s\r\n", str);
      while (*value) 
        {
        if (*value == '(')
          nesting++;
        else if (*value == ')') {
          if (nesting)
            nesting--;
          } else if (*value == '=')
            if (0 == nesting)
              break;
        value++;
        }
      yield();
      if (*value)
        {
          *value = '\0' ;                              // Separate command from value
          analyzeCmdRR ( cmdReply, s, value + 1, returnFlag ) ;        // Analyze command and handle it
          *value = '=' ;                               // Restore equal sign
        }
      else
        {
          analyzeCmdRR ( cmdReply, s, "", returnFlag ) ;              // No value, assume zero
        }
      yield();
      commands_executed++ ;
      s = next ;
    }
    if (commands_executed > 1)
      snprintf ( cmdReply, sizeof ( cmdReply ), "Executed %d command(s) from sequence %s", commands_executed, cmds ) ;
    free ( cmds ) ;
  }
  return cmdReply ;
}



const char* analyzeCmdsRR( String s) {
  bool returnFlag = false;
  return analyzeCmdRR ( s.c_str() , returnFlag );
}

const char* analyzeCmdsRR( String s, bool& returnFlag) {
  return analyzeCmdRR ( s.c_str() , returnFlag );
}




const char* analyzeCmdsFromKey ( const char * key )
{
  if (key)
    if (strlen(key))
    {
    String commands = ramsearch(key)?ramgetstr(key) : (nvssearch(key)?nvsgetstr(key):"");
      if (commands.length())
      {
        commands.trim();
        dbgprint("Now execute commands (from key=%s): %s" , key, commands.c_str());
        return analyzeCmd(commands.c_str());
      }
    }
  return "";
};

const char* analyzeCmdsFromKey ( String key )
{
  return analyzeCmdsFromKey(key.c_str());
}


#endif //if !defined(WRAPPER)



int searchGenre(const char * name)
{
  int ret = genres.findGenre(name);
  if (ret > 0)
  {
    if (genres.count(ret) == 0)
      ret = 0;
  }
  else
    ret = 0;
  return ret;
}

bool doDelete(int idx, String genre, bool deleteLinks, String& result)
{  
  int storedId = 0;
  bool ret = false;

  storedId = searchGenre(genre.c_str());
  if (0 == storedId)
  {
    result = String("Error: genre not found on file.");
  }
  else if (idx != storedId)
  {
    result = String("Error: genre id does not match any on file.");
  }
  else
  {
    if ( (ret = genres.createGenre(genre.c_str(), deleteLinks)) )
      result = String("OK");
    else
      result = String("Error: could not create Genre. LITTLEFS full?");
  }
  
  return ret;
}

bool playGenre(int id)
{
  if (0 == id)
  {
    genreId = genrePresets = genrePreset = 0;
    if (gverbose)
      doprint("Genre playmode is stopped");
    return true;
  }

  int numStations = genres.count(id);
  char cmd[50];
  if (numStations == 0)
    return false;
  
  genreId = id;
  genrePresets = numStations ;
  genrePreset = random(genrePresets) ;
  if (gverbose)
    doprint("Active genre is now: '%s' (id: %d), random start gpreset=%d (of %d)",
        genres.getName(id).c_str(), id, genrePreset, genrePresets);
  sprintf(cmd, "gpreset=%d", genrePreset);
  //analyzeCmd(cmd);
  doGpreset(String(genrePreset));
  return true;
}

void doGenreConfig(String param, String value)
{
  bool ret = true;
  if (param == "rdbs")
    genres.config.rdbs(value.c_str());
  else if (param == "noname")
    genres.config.noName(value.toInt());
  else if (param == "showid")
    genres.config.showId(value.toInt());
  else if (param == "verbose")
    genres.config.verbose(value.toInt());
  else if (param == "usesd")
    genres.config.useSD(value.toInt());
  else if (param == "path")
    genres.nameSpace(value.c_str());
  else if (param == "store")
    genres.config.toNVS();
  else if (param == "info")
    genres.config.info();
  else
    ret = false;
  if (!ret)
    genres.dbgprint("Unknown genre config parameter '%s', ignored!", param.c_str());
}

String doGenre(String param, String value)
{
  String ret = "";
  if (isAnnouncemode())
    return "Command 'genre' not allowed in Announce-Mode";

  if (value.startsWith("--")) 
  {
    //static bool gverbosestore;
    const char *s = value.c_str() + 2;
    const char *remainder = strchr(s, ' ');
    if (remainder) 
    {
      param = String(s).substring(0, remainder - s) ;
      remainder++;
      value = String(remainder);
      chomp(value);
    }
    else
    {
      value = "";
      param = String(s);
    }
    if (gverbose)
      doprint("Genre controlCommand --%s ('%s')",
        param.c_str(), value.c_str());
    if (param == "id")
    {
      if (isdigit(value.c_str()[0]))
        playGenre(value.toInt());
      ret = String(dbgprint("Current genre id is: %d", genreId));
    }
    else if (param == "preset")
    {    
      ret = doGpreset ( value );
    }
    else if (param.startsWith("verb"))
    {
      if (isdigit(value.c_str()[0]))
        genres.config.verbose(value.toInt());//gverbose = value.toInt();
      genres.dbgprint("is in verbose-mode.");
      ret="TODO: genres.config.verbose()";
    }
    else if (param == "stop")
    {
      playGenre(0);
      genres.dbgprint("stop playing from genre requested.");
      ret = "TODO: playGenre(0)";
    } 
    else if (param == "deleteall")
    {
      genres.deleteAll();
      ret = "TODO: genres.deleteAll()";
    }
/*    
    else if (param == "load")
    {
      addToTaskList(GNVS_TASK_LOAD, value);
    }
    else if (param.startsWith("push"))
    {
      doprint("Pushback: '%s'", value.c_str());
      if (value.length() > 0) 
        gnvsTaskList.push(new GnvsTask(value.c_str(), GNVS_TASK_PUSHBACK));
    }
    else if (param.startsWith( "part")) 
    {
      partGenre();
    }
    else if (param == "cmd")
    {
      if (value.length() > 0) 
        analyzeCmd(value.c_str());
    }
*/  
    else if (param == "create")
    {
      genres.createGenre(value.c_str());
      ret = "TODO: genres.createGenre";
    }
    else if (param == "delete")
    {
      genres.deleteGenre(value.toInt());
      ret = "TODO: genres.deleteGenre";
    }
    else if (param == "find")
    {
      int id = genres.findGenre(value.c_str());
      ret = String(doprint("Genre '%s'=%d", value.c_str(), id));
    }
    else if (param == "format")
    {
      genres.format(value.toInt());
      ret = "TODO: genres.format()";
    }
/*   
    else if (param == "add")
    {
      char *s = strchr(value.c_str(), ',');
      if (s)
        s = s+1;
      genres.add(value.toInt(), s);
    }
*/
    else if (param == "count")
    {
      int ivalue = value.toInt();
      ret = doprint("Number urls in Genre with id=%d is: %d", ivalue, genres.count(ivalue));
    }
    else if (param == "fill")
    {
      for (int i = 0;i < 1000;i++)
      {
        char key[50];
        sprintf(key, "genre%d", i);
        genres.createGenre(key);
      }
      ret = "TODO genre=fill";
    }
    else if (param == "url")
    {
      uint16_t nb;
      char *s = strchr(value.c_str(), ',');
      if (s)
        nb = atoi(s+1);
      else
        nb = 0;
      ret = String(doprint("URL[%d] of genre with id=%d is '%s'", nb, value.toInt(), genres.getUrl(value.toInt(), nb).c_str()));
    }
    else if (param == "addchunk")
    {
      const char *s = value.c_str();
      int id;
      if (1 == sscanf(s, "%d", &id))
      {
        int idx = strspn(s, "1234567890");
        if ((idx > 0) && (s[idx] != 0))
        {
            doprint("AddChunk '%s' to genre=%d, delimiter='%c'", s + idx + 1, id, s[idx]);
            genres.addChunk(id, s + idx + 1, s[idx]);
        }
      }
      ret="OK";
    }
    else if (param == "ls")
    {
      genres.ls();
      ret = "TODO: genres.ls()";
    }
    else if (param == "lsjson")
    {
      //const char *s = param.c_str() + 6;
      genres.dbgprint("List genres as JSON, full=%d", value.toInt());
      genres.lsJson(Serial, value.toInt());
      ret = "TODO: List genres as JSON";
    }
    else if (param == "test")
    {
      genres.test();
      ret = "TODO: genres.test()";
    }

/*    else if (param == "show")
    {
      nvs_iterator_t it = nvs_entry_find("presets", "presets", NVS_TYPE_ANY);
      if (gverbose)
        while (it != NULL) 
        {
          nvs_entry_info_t info;
          nvs_entry_info(it, &info);
          it = nvs_entry_next(it);
          doprint("key '%s', type '%d' \n", info.key, info.type);
       }
    } 
    else if (param.startsWith("maint"))
    {
      if (isdigit(value.c_str()[0]))
      {
        bool request = value.toInt() != 0;
        if ( request != gmaintain )
        {
          gmaintain = request;
          if (request)
          {
            vTaskSuspend( xplaytask );
            //vTaskSuspend ( xspftask );
            gverbosestore = gverbose;
            gverbose = true;
          }
          else
          {
            vTaskResume( xplaytask );
            //vTaskResume ( xspftask );
            gverbose = gverbosestore;
          }
        }
      }
    }*/
  }
  else
  {
    if (value.length() == 0) 
    {
      ret = doGenre("genre", "--stop");
    }
    else
    {
      int id = searchGenre(value.c_str());
      if (id)
        ret = playGenre(id);
      else
      {
        ret = "Error: Genre '" + value + "' is not known." ;
      }
    }
  }
  // else   gnvsTaskList.push(new GnvsTask(value.c_str(), GNVS_TASK_OPEN));
  return ret;

}

String readGenrePlaylist(void *)
{
  return genres.playList();
}

String readGenreName(void *)
{
  return genres.getName( genreId );
}



String doGpreset(String value)
{
  String ret = "";
  if (isAnnouncemode())
    return "Commmand 'gpreset' not allowed in Announce-Mode";
  dbgprint("doGpreset started with value=%s", value.c_str());
  /*  
  extern TaskHandle_t maintask;
  StackType_t stackSize = uxTaskGetStackSize(maintask);
 
  dbgprint("Task stack size is: %d", stackSize);

  TaskStatus_t xTaskDetails;
 
    vTaskGetInfo( // The handle of the task being queried. 
                  NULL,
                  // The TaskStatus_t structure to complete with information
                  //on xTask. 
                  &xTaskDetails,
                  // Include the stack high water mark value in the
                  //TaskStatus_t structure. 
                  1,
                  // Include the task state in the TaskStatus_t structure. 
                  0 );
  */
  int ivalue = value.toInt();
  if (genreId != 0) 
  {
    if (ivalue >= genrePresets)
      ivalue = ivalue - genrePresets * (ivalue / genrePresets) ;
    else if (ivalue < 0)
      ivalue = ivalue + genrePresets * (1 + (-ivalue - 1) / genrePresets) ;

    if (ivalue < genrePresets) 
    {
      char key[20];
      String s, s1, temp;
      int idx;
      sprintf(key, "%d_%d", genreId, ivalue);
      genrePreset = ivalue;
      //s = gnvsgetstr(key);
      dbgprint("Try to get URL for genreId=%d, ivalue=%d", genreId, ivalue);
      temp = s = genres.getUrl(genreId, ivalue, false);
      genres.dbgprint("URL#Station found: %s", s.c_str());
      idx = s.indexOf('#');
      if (idx >= 0)
      {
        s1 = s.substring(idx + 1);
        s = s.substring(0, idx);
      }
      if (gverbose) 
      {
        if (idx >= 0)
        {
          ret = dbgprint("Switch to GenreUrl: '%s', Station name is: '%s'", s.c_str(), s1.c_str());
        }
        else
          ret = dbgprint("Switch to GenreUrl: '%s', Station name is not on file!", s.c_str());

      }
      if (s.length() > 0)
      {
        char cmd[s.length() + 10];
        sprintf(cmd, "station=%s", s.c_str());
        ini_block.newpreset = currentpreset;
        analyzeCmd(cmd);
        knownstationname = s1;
        //setLastStation(temp);
        //lastStation = temp;
//        host = s;
//        connecttohost();
      }
      else
      {
      if (gverbose)
        ret = doprint("BUMMER: url not on file");

      }
    }
    else
    {
      if (gverbose)
        ret = doprint("Id %d is not in genre:presets(%d)", ivalue);
    }
  }
  else
  {
    ret = dbgprint("BUMMER: genrePreset is called but no genre is selected!");
  }
  return ret;
}

String urlDecode(String &SRC) {
    String ret;
    //char ch;
    int i, ii;
    for (i=0; i<SRC.length(); i++) {
        if (SRC.c_str()[i] == 37) {
            sscanf(SRC.substring(i+1,i + 3).c_str(), "%x", &ii);
            //ch=static_cast<char>(ii);
            //doprint("urldecode %%%s to %d", SRC.substring(i+1,i + 3).c_str(), ii);
            ret = ret + (char)ii;
            i=i+2;
        } else {
            ret+=SRC.c_str()[i];
        }
    }
    return (ret);
}

/*
int createEmptyGenre(const char* name, const char* firstLink)
{
  //ToDo: Link-Liste anlegen!?
  return genres.createGenre(name);
  gnvsopen();
  if (!gnvshandle)
    return 0;
  bool done = false;
  int tableId = 0;
  int idx;
  char key[30];
  do
  {
    if (done = !loadGnvsTableCache(++tableId))
    {
      createGnvsTableCache();
      idx = 1;
    }
    else
      for (idx = 0;!done && (idx <= GENRE_TABLE_ENTRIES);)
      done = (0 == gnvsTableCache.entry[idx++ % GENRE_TABLE_ENTRIES].timeMode);
  } while ( !done );
  gnvsTableCache.entry[(idx - 1)].timeMode = 2021;
  gnvsTableCache.entry[(idx - 1)].presets = 0;
  strncpy(gnvsTableCache.entry[(idx - 1)].name, name, GENRE_IDENTIFIER_LEN);
  gnvsTableCache.entry[(idx - 1)].name[GENRE_IDENTIFIER_LEN] = 0;
  writeGnvsTableCache();
  idx = idx + GENRE_TABLE_ENTRIES * (gnvsTableCache.id - 1);
  String linkEntry;
  if (firstLink)
    if (0 == strcmp(name, firstLink))
      firstLink = NULL;
  if (firstLink)
    linkEntry = String(firstLink);
  linkEntry.trim();
  sprintf(key, "%d_x", idx);
  doprint("Generating genre link key %s='%s'", key, linkEntry.c_str());
  gnvssetstr (key, linkEntry);
  return idx;
}
*/

bool canAddGenreToGenreId(String idStr, int genreId)
{
  String knownLinks = genres.getLinks(genreId);
  if (knownLinks.length() == 0)
  {
    genres.addLinks(genreId, idStr.c_str());
    return true;
  }  
  do {
    if (idStr == split(knownLinks, ','))
      return false;
  } while (knownLinks.length() > 0);
  genres.addLinks(genreId, idStr.c_str());
  return true;
}



#define BASE64PRINTER_BUFSIZE       1500        // multiple of 3!!!!

class Base64Printer: public Print {
public:  
  Base64Printer(Print& myPrinter) : _myPrinter(myPrinter), _step(0) {
  };

  ~Base64Printer() {
    done();
  }

  void done() {
    if (_step !=0 )
    {
      size_t l = Base64.encodedLength(_step);
      uint8_t encodedString[l];
      Base64.encode((char *)encodedString, (char *)_buf, _step);
      _myPrinter.write(encodedString, l);
      _step=0;
    }
  };

  size_t write(uint8_t byte) {
    //Serial.write(byte);
    _buf[_step++] = byte;
    if (_step == BASE64PRINTER_BUFSIZE) {
      size_t l = Base64.encodedLength(BASE64PRINTER_BUFSIZE);
      uint8_t encodedString[l];
      Base64.encode((char *)encodedString, (char *)_buf, BASE64PRINTER_BUFSIZE);
      _myPrinter.write(encodedString, l);
      _step = 0;
    }
    return 1;
  };

protected:
  Print& _myPrinter;  
  static const int bufsize;
  uint8_t _buf[BASE64PRINTER_BUFSIZE];
  uint16_t _step;
};


void httpHandleGenre ( String http_rqfile, String http_getcmd ) 
{
String sndstr = "";
  http_getcmd = urlDecode (http_getcmd) ;
  //doprint("Handle HTTP for %s with ?%s...", http_rqfile.c_str(), http_getcmd.substring(0,100).c_str());
  if (http_rqfile == "genre.html")
  {
    sndstr = httpheader ( String ("text/html") );
    cmdclient.println(sndstr);
    //cmdclient.println(genre_html);
    const char *p = genre_html;
    int l = strlen(p);
    while ( l )                                       // Loop through the output page
    {
      if ( l <= 1024 )                              // Near the end?
      {
        cmdclient.write ( p, l ) ;                    // Yes, send last part
        l = 0 ;
      }
      else
      {
        cmdclient.write ( p, 1024 ) ;         // Send part of the page
        p += 1024 ;                           // Update startpoint and rest of bytes
        l -= 1024 ;
      }
    }
    cmdclient.println("\r\n");                        // Double empty to end the send body
  }
  else if (http_rqfile == "genredir")
  {
    Base64Printer base64Stream(cmdclient);
    doprint("Sending directory of loaded genres to client");
    sndstr = httpheader ( String ("text/json") ) ;
    cmdclient.print(sndstr);
    //dirGenre (&cmdclient, true) ;
    //genres.lsJson(cmdclient);
    genres.lsJson(base64Stream);
    base64Stream.done();
    cmdclient.println("\r\n\r\n");                        // Double empty to end the send body
  }
  else if (http_rqfile == "genre")
  {
    int genreId = http_getcmd.toInt();
    //bool dummy;
    http_rqfile = String("--id ") + genreId;
    doprint("Genre switch by http:...%s (id: %d)", http_rqfile.c_str(), genreId);
    if (genreId > 0)
      doGenre("genre", http_rqfile);
    sndstr = httpheader("text/html") + "OK\r\n\r\n";
    cmdclient.println(sndstr);
  }
  else if (http_rqfile == "genreaction")
  {
    sndstr = httpheader ( String ("text/text")) ;
    cmdclient.println(sndstr);
    
    int decodedLength = Base64.decodedLength((char *)http_getcmd.c_str(), http_getcmd.length());
    char decodedString[decodedLength];
    Base64.decode(decodedString, (char *)http_getcmd.c_str(), http_getcmd.length());
    http_getcmd = String(decodedString);
    genres.dbgprint("Running HTTP-genreaction with: '%s'%s", 
      http_getcmd.substring(0, 100).c_str(), (http_getcmd.length() > 100?"...":""));
    //Serial.print("Decoded string is:\t");Serial.println(decodedString);    
    

    String command = split(http_getcmd, '|');
    String genre, idStr;
    int idx = command.indexOf("genre=");
    if (idx >= 0) 
    {
      String dummy = command.substring(idx + 6);
      genre = split(dummy, ',');
      genre.trim();
    }
    if (command.startsWith("link="))
    {
      sndstr = "OK";
    }    
    else if (/*command.startsWith("link") || */command.startsWith("link64="))
    {
      String dummy = command.substring(7);
      idStr = split(dummy, ',');
      idStr.trim();
      int genreId = idStr.toInt();
      //int genreId = searchGenre(genre.c_str());
      if (0 == genreId)
      {
        sndstr =  "ERR: Could not load genre '" + genre + "' to get links...";
      }
      else
      {
        /*
        char key[30];
        sprintf(key, "%d_x", genreId);
        sndstr = gnvsgetstr(key) + "\r\n\r\n"; */
        sndstr = genres.getLinks(genreId);// + "\r\n\r\n";
        //doprint("GenreLinkResult: %s", sndstr.c_str());
      }
      if (command.startsWith("link64"))
      {
        int encodedLength = Base64.encodedLength(sndstr.length() + 1);        
        char encodedString[encodedLength];
        Base64.encode(encodedString, (char *)sndstr.c_str(), sndstr.length());
        sndstr = String(encodedString);// + "\r\n\r\n";
      }  
    }
    else if (command.startsWith("createwithlinks"))
    {
      int genreId;
      if ((genreId =genres.createGenre(genre.c_str())))
      {
        genres.dbgprint("Created genre '%s', Id=%d, links given with len=%d (%s)",
                genre.c_str(), genreId, http_getcmd.length(), http_getcmd.c_str());
        genres.addLinks(genreId, http_getcmd.c_str());
      }
      else
        genres.dbgprint("Error: could not create genre '%s' (HTTP: createwithlinks", genre.c_str());
    }
    else if (command.startsWith("nvsgenres"))
    {
      sndstr = nvsgetstr("$$genres");
      chomp(sndstr);
      int encodedLength = Base64.encodedLength(sndstr.length() + 1);        
      char encodedString[encodedLength];
      Base64.encode(encodedString, (char *)sndstr.c_str(), sndstr.length());
      sndstr = String(encodedString);// + "\r\n\r\n";
    }
    else if (command.startsWith("del=") /*|| (command.startsWith("clr="))*/)
    {
      String dummy = command.substring(4);
      idStr = split(dummy, ',');
      idStr.trim();
      //bool deleteLinks = command.c_str() [0] == 'd' ;
      doprint("HTTP is about to delete genre '%s' with id %d", genre.c_str(), idStr.toInt());
      //doDelete(idStr.toInt(), genre, deleteLinks, sndstr);
      genres.deleteGenre(idStr.toInt());
      //delay(2000);
      sndstr = "Delete done, result is:"  + sndstr;
      genres.dbgprint(sndstr.c_str());
      sndstr = "OK";//\r\n\r\n";
    }
    else if (command.startsWith("save=") || (command.startsWith("add=")))
    {
      int genreId;
      bool isAdd = command.startsWith("add=");
      bool isStart = false;
      const char *s;
      String dummy = command.substring(isAdd?4:5);
      idStr = split(dummy, ',');
      idStr.trim();
      int count=-1;int idx = -1;
      s = strstr(command.c_str(), "count=");
      if (s)
        count = atoi(s + 6);
      s = strstr(command.c_str(), "idx=");
      if (s)
        idx = atoi(s + 4);
      s = strstr(command.c_str(), "start=");
      if (s)
        isStart = atoi(s + 6);
      if (isStart)
      {
        if (0 < (genreId = genres.findGenre(genre.c_str())))
        {
            if (isAdd) 
            {
              if (genres.count(genreId) == 0)
              {
                genres.createGenre(genre.c_str(), true);
                genres.dbgprint("First deleting genre: '%s' (also deleting links=%d)", genre.c_str(), 1);
              }
            }
            else
            {
              genres.dbgprint("First deleting genre: '%s' (also deleting links=%d)", genre.c_str(), idStr == "undefined");
              genres.createGenre(genre.c_str(), idStr == "undefined");
            }
        }
        else
        {
            genres.dbgprint("Creating empty genre: '%s'.", genre.c_str());
            genres.createGenre(genre.c_str());
        }
      }
      genreId = genres.findGenre(genre.c_str());
      sndstr = "OK";//\r\n\r\n";
      if (genreId)
      {
        bool fail = false;
        if (isAdd && (idx == 0))                  // possibly a new subgenre to add to the main genre?
          if (!isStart || (genre != idStr))       // special case: genre name is identical to first subgenre
            if (!canAddGenreToGenreId(idStr, genreId))
            {
              sndstr = "Error: can not add genre " + idStr + " to genre " + genre ;
              genres.dbgprint(sndstr.c_str());
              //sndstr = sndstr + "\r\n\r\n";
              fail = true;
            }
        if (!fail)
        {
          genres.dbgprint("Adding %d presets to genre '%s'.", count, genre.c_str());
          genres.addChunk(genreId, http_getcmd.c_str(), '|');
        }  
      } // if genreId
      else
      {
        sndstr = "Error: genre " + genre + " not found in Flash!";
        genres.dbgprint(sndstr.c_str());
        //sndstr = sndstr + "\r\n\r\n";
      }
#ifdef OLDOLDOLD        
      if ((idx >= 0) && (count > 0))
      {
        int genreId = searchGenre(genre.c_str());
        if (isAdd && (genreId > 0) && (idx == 0))
          genreId = 0;
        doprint("Add %d presets to genre '%s' starting at index %d.", count, genre.c_str(), idx);
        sndstr="OK\r\n\r\n";        
        if (idx == 0)
        {
          if (genreId != 0)
          {
            if (!isAdd)
            {
              sndstr = String(doprint("Genre '%s' is already known. Use refresh to reload.", genre.c_str())) 
                + "\r\n\r\n";
              genreId = 0;
            }
            else 
            {
              if (genre == idStr)
              {
                sndstr = String(doprint("Huh? Can not add genre '%s' to itself.", genre.c_str())) + "\r\n\r\n";
                genreId = 0;
              }
              else if (!canAddGenreToGenreId(idStr, genreId))
              {
                char *s = doprint("Error: can not add genre '%s' to '%s' twice.", idStr.c_str(), genre.c_str());
                sndstr = String(s) + "\r\n\r\n";
                genreId = 0;
              }
            }
          }
          else
          {
            genreId = createEmptyGenre(genre.c_str(), false);//isAdd?idStr.c_str():NULL);
            doprint("Creating empty genre for %s:%d", genre.c_str(), genreId);
          }
        }
        if (0 != genreId)
          {
            genres.addChunk(genreId, http_getcmd.c_str(), '|');
            /*
            int presets = gnvsTableCache.entry[(genreId - 1) % GENRE_TABLE_ENTRIES].presets;
            for (int i = 0;i < count;i++,presets++)
            {
              char key[30];
              String preset = getStringPart(http_getcmd, '|');
              sprintf(key, "%d_%d", genreId, presets);
              gnvssetstr(key, preset);
              //doprint("%3d: %5s=%s", presets, key, preset.c_str());
            }
            gnvsTableCache.entry[(genreId - 1) % GENRE_TABLE_ENTRIES].presets = presets;
            writeGnvsTableCache();
            */
        
          }
        else if (sndstr.startsWith("OK"))
          {
            sndstr =  String ( doprint("Could not load or create genre '%s' to add to...", genre.c_str()) ) + "\r\n\r\n";
          }
      }
      else
      {
        doprint("Error: nothing to add to genre '%s' (idx==%d, count==%d)", genre.c_str(), idx, count);
        sndstr = "ER\r\n\r\n";
      }
#endif
    }
    else if (command.startsWith("setconfig="))
    {
      DynamicJsonDocument doc(JSON_OBJECT_SIZE(20));
      DeserializationError err = deserializeJson(doc, command.c_str() + strlen("setconfig="));
      if (err == DeserializationError::Ok)
      {
        const char* path = doc["path"];
        const char* rdbs = doc["rdbs"];
        int noname = doc["noname"];
        int verbose = doc["verbose"];
        int showid = doc["showid"];
        int save = doc["save"];
        doGenreConfig("verbose", String(verbose));
        doGenreConfig("rdbs", String(rdbs));
        doGenreConfig("path", String(path));
        doGenreConfig("noname", String(noname));
        doGenreConfig("showid", String(showid));
        if (save)
          genres.config.toNVS();
      }
    }
    else
    {
      sndstr = "Unknown genre action '" + command + "'\r\n\r\n";
    }
    if (sndstr.lastIndexOf("\r\n\r\n") < 0)
      sndstr += "\r\n\r\n";
    if (sndstr.length() > 0) 
    {
        genres.dbgprint("CMDCLIENT>>%s", sndstr.substring(0,500).c_str());
        cmdclient.println(sndstr);
        cmdclient.flush();
        //delay(20);
    }
    //nvs_commit(gnvshandle);
  }
  else if (http_rqfile == "genrelist" )
  {
    Base64Printer base64Stream(cmdclient);
    doprint("Sending directory of loaded genres to (remote) client");
    sndstr = httpheader ( String ("application/json") ) ;
    cmdclient.print(sndstr);
    //dirGenre (&cmdclient, true) ;
    //genres.lsJson(cmdclient);
    genres.lsJson(base64Stream, LSMODE_SHORT|LSMODE_WITHLINKS);
    base64Stream.done();
    cmdclient.println("\r\n\r\n");                        // Double empty to end the send body
  }
  else if (http_rqfile == "genreconfig")
  {
    sndstr = httpheader ( String ("application/json") ) ;
    cmdclient.print(sndstr);
    sndstr = genres.config.asJson();
    genres.dbgprint("Sending config: '%s'", sndstr.c_str());
    cmdclient.println(sndstr);
    cmdclient.println("\r\n\r\n");                        // Double empty to end the send body
  }
  else if (http_rqfile == "genreformat")
  {
    sndstr = httpheader ( String ("text/text") );
    cmdclient.println(sndstr);

    cmdclient.println("OK\r\n");
    if (genres.format(true))
      cmdclient.println("OK: LITTLEFS formatted for genre info");
    else
      cmdclient.println("Error: could not format LITTLEFS for genre info.");
    cmdclient.println();
    cmdclient.println();
  }
  else
  {
    sndstr = httpheader ( String ("text/text") );
    cmdclient.println(sndstr);
    cmdclient.printf("Unknown file?command request with %s?%s.\r\n\r\n\r\n", 
                        http_rqfile.c_str(), http_getcmd.c_str());                        // Double empty to end the send body
  }
  cmdclient.stop();
}

bool favPresent[101];
bool httpfavlistdirty = true;
String httpfavlist;
uint16_t favlistversion = 0;
int mqttfavidx = 0;
int mqttfavendidx = 0;

void setmqttfavidx(int idx, int rangeEndIndex) 
{
        if (mqttfavidx == 0)
        {
          mqttfavidx=idx;
          mqttfavendidx=rangeEndIndex;
        }
        else
        {
          if (mqttfavidx > idx)
            mqttfavidx = idx;
          if (mqttfavendidx < rangeEndIndex)
            mqttfavendidx = rangeEndIndex;
        }
}

String readfavfrompref ( int16_t idx )
{
  String s;
  char           tkey[12] ;                            // Key as an array of char
  if ((idx < 1) || (idx > 100))
    return "";
  favPresent[idx] = false;
  sprintf ( tkey, "fav_%02d", idx ) ;              // Form the search key
  if ( !nvssearch ( tkey ) )                           // Does _x[x[x]] exists?
  {
      //Serial.printf("Favorite Key('%s') not in preferences!" , tkey);
      return String ( "" ) ;                           // Not found
  }
  // Get the contents
  
  s = nvsgetstr ( tkey, false ) ;                          // Get the station (or empty sring)
  s.trim();
  favPresent[idx] = s.length() > 0;
  return s;
}


void setupReadFavorites()
{
  for(int i = 1;i < 101;i++)
  {
    readfavfrompref(i);
  }
  favlistversion = 1;
}

void readhttpfavlist()
{
  bool first = true;
  httpfavlist = "[";
  for(int i = 1;i < 101;i++)
    if (favPresent[i])
    {
      String s = readfavfrompref(i);
      if (s.length() > 0)
      {
        int idx = s.indexOf('#');
        if (idx >= 0)
          s = s.substring(idx + 1);
        s.trim();
        /*
        int delim = s.indexOf('"');
        while (delim >= 0)
        {
          s = s.substring(0, delim) + "'" + s.substring(delim + 1);
          delim = s.indexOf('"');
        } 
        */
        if (s.length() > 50)
          s = s.substring(0, 50);
        size_t l = Base64.encodedLength(s.length());
        uint8_t encodedString[l];
        Base64.encode((char *)encodedString, (char *)s.c_str(), s.length());        
        s = String((char *)encodedString);
        if (!first)
          httpfavlist = httpfavlist + ",";
        httpfavlist = httpfavlist + String("{\"i\":") + i + ",\"n\":\"" + s + String("\"}");
        first = false;
      }
    }
  httpfavlist = httpfavlist + "]";
  httpfavlistdirty = false;
}

String gethttpfavlist()
{
  if (httpfavlistdirty)
    readhttpfavlist();
  return String("{\"version\": ") + favlistversion + ", \"list\": " + httpfavlist + "}";
}


bool getFavRange(const char *s, int& rmin, int& rmax, bool& haveNumber)
{
  rmin = 1;
  rmax = 100;
  haveNumber = false;
  const char *s1 = s+1;
  while (*s1 && (*s1 <= ' '))
    s1++;
  if (!(haveNumber = isdigit(*s1)))
    return true;
  rmin=atoi(s1);
  if ((rmin < 1) || (rmin > 100))
    return false;
  rmax=rmin;
  while(isdigit(*s1))
    s1++;
  while (*s1 && (*s1 <= ' '))
    s1++;
  //Serial.printf("Next part of range: %s\r\n", s1);
  if (*s1 == '-')
  {
    s1++;
    while (*s1 && (*s1 <= ' '))
      s1++;
  }
  //Serial.printf("Search for number in remaining: %s\r\n", s1);
  if (!isdigit(*s1))
    return true;
  //Serial.printf("Resolve to rmax number: %s\r\n", s1);
  rmax = atoi(s1);
  return (rmax >= rmin) && (rmax <= 100);
}

String deleteFavorite(int i)
{
char tkey[12];
  if (!favPresent[i])
    return String("Favorite[") + i + "] is not defined.";
  httpfavlistdirty = true;
  favlistversion++;
  //decrease favorite count if needed
  favPresent[i] = false;
  if (currentFavorite == i)
  {
    currentFavorite = 0;
    mqttpubFavNotPlaying();
  }
  sprintf(tkey, "fav_%02d", i);
  nvsdelkey ( tkey ) ;
  setmqttfavidx(i, i);
  //setLastStation(currentStation);
  //favplayrequestinfo("Rescan after delete!", true);
  return String("Favorite[") + i + "] deleted successfully!";
}

String addFavorite(int i)
{
String ret;
char tkey[12];
  if (!favPresent[i])
    ;       // increase favcount if needed...
  favPresent[i] = true;
  httpfavlistdirty = true;
  favlistversion++;
  sprintf(tkey, "fav_%02d", i);
  nvssetstr ( tkey, host ) ; //currentStation ) ;
  setmqttfavidx(i, i);
  currentFavorite = i;
  return String("Added Favorite as number ") + i;
}

String getFavoriteJson(int idx, int rMin, int rMax)
{
  if ((idx != 0) && ((idx < rMin) || (idx > rMax)))
  //if ((idx < 0) || (idx > 100))
    return "";
  String favinfo = String("{\"idx\": \"") + idx;
  String url = readfavfrompref ( idx );
  //Serial.printf("readfavfrompref(%d) is: '%s'\r\n", idx, url.c_str());
  String lastHost = host ; //currentStation;
  chomp(lastHost);
  String name;
  int delim = url.indexOf('#');
  if (delim >= 0)
  {
    name = url.substring(delim + 1);
    url = url.substring(0, delim );
  }
  else
    name = url;
  name.trim();url.trim();
  delim = name.indexOf('"');
  while (delim >= 0)
  {
    name = name.substring(0, delim) + "'" + name.substring(delim + 1);
    delim = name.indexOf('"');
  }
  bool play = (url == lastHost) || ((0 == idx));// && ((currentFavorite < rMin) || (currentFavorite > rMax)));
  favinfo = favinfo + "\", \"url\":\"" + url + "\", \"name\":\"" + name + "\", \"play\":\"" + (play?"1":"0") + "\"}"; 
//  if (play)
//    favplayreport(url);
  return favinfo;  
}

String doFavorite (String param, String value)
{
  char c;
  int rMin, rMax;
  bool numberGiven;
  const char* value_cstr = value.c_str();
  String ret = "";
  if (isAnnouncemode())
    return "Command 'favorite' not allowed in Announce-Mode";
  if (param != "favorite")
    return "Unexpected command word: " + param;
  c = tolower(*value_cstr);
  if (isDigit(c))
  {
    int idx = atoi(value.c_str());
    if ((idx > 0) && (idx <= 100))
    {
      if (!favPresent[idx])
        ret = String("Favorite[") + idx + "] is not defined!";
      else
      {
        /*
        String s = readfavfrompref(idx);
        String temp = "";
        int nameIdx = s.indexOf('#');
        if (nameIdx >= 0)
        {
          temp = s.substring(nameIdx + 1);
          temp.trim();
        }
        chomp(s);
        s = "station=" + s;
        */
        String s = "station=" + readfavfrompref(idx);
        ini_block.newpreset = currentpreset;
        analyzeCmd(s.c_str());
        //setLastStation(temp);
        //lastStation = temp;
        currentFavorite = idx;
        //if (nameIdx >= 0)
          //knownstationname = temp;
        ret = String("Favorite is now ") + idx;
      }  
      //setmqttfavidx(idx, idx);
    }
    else
      return String("Favorite number must be between 1 and 100 (not ") + idx + ")";
    return ret;
  }
  if (!getFavRange(value.c_str(), rMin, rMax, numberGiven))
  {
    return String("Illegal fav Range in '") + String(value_cstr + 1) + "'";
  }
  if (value == "jsonlist")
    ret = gethttpfavlist();
  else if (value == "status")
    ret = String("{\"play\": ") + currentFavorite + ", \"version\": " + favlistversion + "}";
  else if (('+' == c) || ('-' == c) || ('?' == c))
  {
    int emptyIndex = 0;
    int foundIndex = 0;
    String currentUrl = host ; //currentStation;
    chomp(currentUrl);
    bool deleted = false;
    for(int i = rMin;i <= rMax;i++)
    {
      if (!favPresent[i])
      {
        if (0 == emptyIndex)
          emptyIndex = i;
      }
      else
      {
        String favUrl = readfavfrompref(i);
        chomp(favUrl);
        if (favUrl == currentUrl)
        {
          if ('-' == c)
          {
            deleted = true;
            deleteFavorite(i);
          }
          else
            if (0 == foundIndex)
              foundIndex = i;
        }
      }
    }
    if ('-' == c)
    {
      if (!deleted)
        ret = String("Current station is not in favorites (range[") + rMin + "-" + rMax + "])";
      else
        ret = "Station deleted from favorites.";
    }
    else if ('+' == c)
    {
      if (0 != foundIndex)
        ret = String("Current station already stored to favorites (range[") +
          rMin + "-" + rMax + "]) as " + foundIndex ;
      else if (0 == emptyIndex)
      {
        mqttpubFavNotPlaying();
        ret = String("No free slot (in range [") + rMin + "-" + rMax + "]) to store new favorite!";
      }
      else
        {
        ret = addFavorite(emptyIndex);
        }
    }
    else if ('?' == c)
    {
      if (0 == foundIndex)
      {
        mqttpubFavNotPlaying();
        ret = String("Current station is not in favorites (range[") + rMin + "-" + rMax + "]";
      }
      else
      {
        //currentFavorite = foundIndex;
        setmqttfavidx(foundIndex, foundIndex);
        ret = String("Current station stored to favorites (range[") +
          rMin + "-" + rMax + "] as " + foundIndex;
      }
    }
  }
  else if ('s' == c)
  {
    if (numberGiven)
      ret = addFavorite(rMin);
    else
      ret = "Favorite not stored: no valid number given!";
  }
  else if ('x' == c)
  {
    if (numberGiven)
      ret = deleteFavorite(rMin);
    else
      ret = "Favorite not deleted: no valid number given!";
  }
  else if ('m' == c)
  {
    setmqttfavidx(rMin, rMax);
    ret = "OK, MQTT pubish for favorites started!";
  }
  else if ('l' == c)
  {
    for(int i = rMin;i <= rMax;i++)
      dbgprint("fav_%02d=%s", i, getFavoriteJson(i, rMin, rMax).c_str());
    ret = "Favorite listing done!";
  }
  return ret;
}

void scanFavorite()
{
String currentUrl = host ; //currentStation;
chomp(currentUrl);
  int foundIndex = 0;
  for (int i = 1;(i < 101) && (0 == foundIndex);i++)
  {
      if (favPresent[i])
      {
        String favUrl = readfavfrompref(i);
        chomp(favUrl);
        if (favUrl == currentUrl)
          foundIndex = i;
      }
  }
  currentFavorite = foundIndex;
 
 /*
  String found = doFavorite("favorite", "?");
  int idx = found.indexOf(" as ");
  if (idx >= 0)
  {
    idx = found.substring(idx + 4).toInt();
  }  
  else
    idx = 0;
  currentFavorite = idx;
*/
}

/*
String favreported;
int favreportindex = 0;

void favplayreport(String url) {
  favreported = url;
  favreportindex = 0;
  Serial.printf("FAVPLAYREPORT: %s\r\n", url.c_str());
}

void favplayrequestinfo(String url, bool rescan) {
  Serial.printf("FAVPLAYREQUEST[%s]:", url.c_str());
  if ((url != favreported) || rescan)
  {
    favreportindex = 1;
    if (!rescan)
      favreported = url;
    Serial.println(" started!");
  }
  else
    Serial.println("OK, same as last reported!");
}

void favplayinfoloop() {
  if (favreportindex)
    if (favreportindex <= 100)
    {
      if (favPresent[favreportindex])
      {
        String url = readfavfrompref(favreportindex);
        chomp(url);
        Serial.printf("FAVCOMPARE[%d]:'%s' ?? '%s'\r\n", favreportindex, 
          url.c_str(), favreported.c_str());
        if (url == favreported)
        {
          currentFavorite = favreportindex;
          setmqttfavidx(favreportindex, favreportindex); 
          Serial.printf("FAVINDEX found: %d\r\n", favreportindex);
          favreportindex = 0;
        }
        else
          favreportindex++;
      }
      else 
        favreportindex++;
    }
    else
    {
      currentFavorite = 0;
      mqttpubFavNotPlaying();
      favreportindex = 0;
      Serial.printf("FAVINDEX not found: Station is not in favorites\r\n");
    }
}
*/


bool UploadFile::begin(String path, bool writeFlag) {
  int idx = 0;
  if (_isOpen)
    return false;
  _littleFSBegun = false;
  _isSD = false;
  path.trim();
  String lpath = path;
  lpath.toLowerCase();
  if (lpath.substring(0, 7) == "file://")
  {
    idx = 7;
    while ((lpath.c_str()[idx] >= 0) && (lpath.c_str()[idx] <= ' '))
      idx++;
  }
  lpath = lpath.substring(idx);
  if (lpath.substring(0, 3) == "sd:")
  {
    _isSD = true;
    idx = idx + 3;
    while ((path.c_str()[idx] >= 0) && (path.c_str()[idx] <= ' '))
      idx++;
  }
  path = path.substring(idx);
  if (path.length() == 0)
    return false;
  if (path.indexOf(' ') >= 0)
    return false;
  if (path.c_str()[0] != '/')
    path = '/' + path;
  if (_isSD)
    path = "/up/l/o/a/d" + path;
  else
    path = "/upload" + path;
  dbgprint("Path to uploadFile is %s%s", _isSD?"SD:":"LittleFS:", path.c_str());
  FS *fs = NULL;
  if (_isSD)
  {
#ifdef SDCARD
    extern bool SD_okey;
    if (SD_okey)
     fs = &SD;
    else
#endif
      dbgprint("SD not available for UPLOAD!");
  }
  else
  {
    if (genres.begun() && (!genres.isSD()))
    {
      fs = &LITTLEFS;
      dbgprint("Reusing LittleFS from Genres");
    }
    else
    {
      dbgprint("Try to open LittleFS");
      if (_littleFSBegun = LITTLEFS.begin())
        fs = &LITTLEFS;
    }
  }
  if (NULL == fs)
    dbgprint("Could not access filesystem for upload file.");
  else
  {
    if (writeFlag)
    {
      char s[path.length() + 1];
      strcpy(s, path.c_str());
      char *p = strrchr(s, '/');
      if (p)
        *p = 0;
      else 
        *s = 0;
      p = *s?s:NULL;
      while (p) 
      {
          p = strchr(p + 1, '/');
          if (p) {
              *p = 0;
          }
          claim_spi();
          bool res = fs->exists(s);
          release_spi();
          if (!res)
          {
              dbgprint("Creating directory '%s' for Sounds", s);
              claim_spi();
              res = fs->mkdir(s);
              release_spi();
          }
          if (!res)
              p = NULL;
          else if (p)
              *p = '/';
        }
      //directory should now exist...
      }

    _soundFile = fs->open(path.c_str(), writeFlag?FILE_WRITE:FILE_READ);  
    if (!(_isOpen = _soundFile))
      dbgprint("Error opening file %s for %s!", path.c_str(),writeFlag?"writing":"reading");
    else
      dbgprint("Success opening file %s for %s!", path.c_str(),writeFlag?"writing":"reading");
    

  }

//DEBUG!!!!
  close();
//DEBUG!!!!



  if (!_isOpen)
    if (_littleFSBegun)
    {
      LITTLEFS.end();
      _littleFSBegun = false;
    }
  return _isOpen;
};

void UploadFile::close() {
  if (!_isOpen)
    return;
  _isOpen = false;
  _soundFile.close();
  if (_littleFSBegun)
  {
    LITTLEFS.end();
    _littleFSBegun = false;
  }
}


void UploadFile::claim_spi() {if (_isSD) claimSPI("sound");};            
void UploadFile::release_spi() {if (_isSD) releaseSPI();};         


UploadFile uploadfile;

#else


#endif // Retroradio

