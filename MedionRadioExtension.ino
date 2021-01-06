#include <nvs.h>
#define NVS_ERASE     true            // NVS will be completely deleted if set to true (only use for development, must be false in field SW)
#define NUMCHANNELS   15
#define NUMWAVEBANDS   3
#define MP3WAVEBAND    255
#define MINVOLUME       50
#define MAXVOLUME      100


//#define TOUCHLEVEL_TONE       450     // pressed detected if below this level
#define TOUCHLEVEL_TONEDIRECT 400     // primary area, if pressed and below that level

#define TOUCHTIME_TONELONG1   400
#define TOUCHTIME_TONE2PRESS  300
#define TOUCHTIME_TONELONGFF  250
#define TOUCHREAD_MINDELTA     17     // distance from "known maximum" to detect touched
#define TOUCHREAD_DIRECTDELTA  65     // delta to distinguish a direct touch from a touch of surrounding area
                                      // must be bigger than TOUCHREAD_MINDELTA, otherwise pressing on sourrounding
                                      // area will not be detected (can be set to _MINDELTA on purpose).


static uint8_t waveBand = 0; //0xff;
static uint8_t tunedChannel = 0;
static bool apiLock = false;
static bool apiVolumeLock = false;




struct medionExtraPins {
  bool ledInverted;
  uint8_t led;
  uint8_t vol;
  uint8_t wave;
  touch_pad_t capaPin;
  touch_pad_t tonePin;
  touch_pad_t loudUp;
  touch_pad_t loudDn;
} medionPins = 
  {
    .ledInverted = false,
    .led = 27,
    .vol = 33,
    .wave = 35,
//    .capa = 0xff,       // obsolete
    .capaPin = TOUCH_PAD_NUM6, //T4, GPIO14
    .tonePin = TOUCH_PAD_NUM5, //T5, GPIO12
    .loudUp = TOUCH_PAD_MAX, //T5,
    .loudDn = TOUCH_PAD_MAX  //T6
  };

bool medionExtensionSetupDone = false;


std::vector<char *> equalizers;
static int currentEqualizer = 0;

void addEqualizer(String value) {
  char *p = strdup(value.c_str());
  if (p) {
    dbgprint("Add equalizer[%d]: %s", equalizers.size(), p);
    equalizers.push_back(p);
  }
}

void setEqualizer(int idx) {
  char *s;
  if (idx < equalizers.size()) {
    s = strdup(equalizers[idx]);
    if (s) {
      currentEqualizer = idx;
      char *p = s;
      while (p) {
        char *p1 = strchr(p, ',');
        if (p1) {
          *p1 = 0;
          p1++;
        }
        analyzeCmd(p);
        p = p1;  
      }
      free(s);
    }
  }
}

uint16_t tpCapaRead() {
  uint16_t x;
  if (medionExtensionSetupDone && (medionPins.capaPin != TOUCH_PAD_MAX))
    touch_pad_read_filtered(medionPins.capaPin, &x);
  else
    x = 0xffff;
  return x;
}

uint16_t tp4read() {
  return tpCapaRead();
}

uint8_t knownPresets = 0;
uint8_t knownPresetList[MAXPRESETS];

struct medionConfigType {
    uint8_t numChannels;
//    uint16_t touchLevelTone;
    uint32_t touchDebounceTime;
    int8_t touchDebounce;
    uint8_t waveSwitchPositions, mp3Waveband;      
}  medionConfig = 
  {
    .numChannels = NUMCHANNELS,
//    .touchLevelTone = TOUCHLEVEL_TONE,
    .touchDebounceTime = 50,
    .touchDebounce = 2,
    .waveSwitchPositions = NUMWAVEBANDS,
    .mp3Waveband = MP3WAVEBAND
  };   

#define MEDION_WAVE_SWITCH_POSITIONS  (medionConfig.waveSwitchPositions)

uint16_t medionSwitchPositionCalibrationDefault[] = {
    4090, 4095,       //WaveBand 0 ganz links
    1750, 2050,        //WaveBand 1 in der Mitte 
        0, 5,         //WaveBand 2 ganz rechts 
};


enum api_event_t {
  API_EVENT_CHANNEL = 0,
  API_EVENT_WAVEBAND,
  API_EVENT_VOLUME,
  API_EVENT_TONE_TOUCH,
  API_EVENT_TONE_UNTOUCH,
  API_EVENT_TONE_PRESSED,
  API_EVENT_TONE_LONGPRESSED,
  API_EVENT_TONE_LONGRELEASED,
  
  API_EVENT_TONE_2PRESSED,
  API_EVENT_TONE_2LONGPRESSED,
  API_EVENT_TONE_2LONGRELEASED,

  API_EVENT_TONE_3PRESSED,
  API_EVENT_TONE_3LONGPRESSED,
  API_EVENT_TONE_3LONGRELEASED
};

enum led_mode_t {
  LED_MODE_LOW = 0,
  LED_MODE_HIGH,
  LED_MODE_TOGGLE,
  LED_MODE_DARK,
  LED_MODE_FORCELOW,
  LED_MODE_FLASH3,
  LED_MODE_FLASH2,
  LED_MODE_FLASH1
};

void toggleMedionLED(led_mode_t ledMode);


uint16_t *medionSwitchPositionCalibration = (uint16_t *)&medionSwitchPositionCalibrationDefault;


#if NUMCHANNELS == 8  
uint16_t medionChannelLimitsDefault8[NUMCHANNELS * 2] = { 
    88 ,98 , 
    105, 117,
    128, 144, 
    158, 181, 
    204, 234, 
    252, 273, 
    294, 310, 
    326, 342
};
uint16_t * medionChannelLimitsNew = (uint16_t *)&medionChannelLimitsDefault8;        

#elif NUMCHANNELS == 15
uint16_t medionChannelLimitsDefault15[NUMCHANNELS * 2] = { 
    80 ,97 , 
    100, 103,
    109, 114, 
    119, 124, 
    132, 140, 
    147, 156, 
    161, 171, 
    181, 202,
    215 ,229 , 
    239, 254,
    260, 271, 
    282, 298, 
    304, 315, 
    320, 330, 
    336, 362
};
uint16_t * medionChannelLimitsNew = (uint16_t *)&medionChannelLimitsDefault15;        

#endif


#define MEDION_DEBUGAPI
//#define MEDIONCALIBRATE_VOLUME
#define MEDIONCALIBRATE_WAVEBAND
//#define MEDIONCALIBRATE_CAPACITOR
#define MEDIONCALIBRATE_TONE

void readPresetList() {
  knownPresets = 0;
  for (int i = 0;i < MAXPRESETS;i++) {
    String s = readhostfrompref(i);
    if (s.length() > 0) 
      knownPresetList[knownPresets++] =  i; 
  }
//  Serial.printf("\r\nKNOWN PRESETS=%d\r\n", knownPresets);
}

void selectRandomPreset() {
  if (knownPresets) {
    toggleMedionLED(LED_MODE_HIGH);
    int i = random(knownPresets);
    char s[20];
    sprintf(s, "preset=%d", knownPresetList[i]);
    analyzeCmd(s);
  }
}

void selectPreset(uint8_t channel = 0xff, bool debugInfo = false) {
  char s[10];
  static uint8_t lastChannel = 0xff;
  toggleMedionLED(LED_MODE_HIGH);
  if (channel != 0xff)
    lastChannel = channel;
  if (lastChannel == 0xff)
    lastChannel = 0;
  if (debugInfo)
    Serial.printf("API switch to channel: %d\r\n", channel);
  sprintf(s, "%d", lastChannel);
  analyzeCmd("preset", s);
}


//#define API_EVENT_CHANNEL  0
//#define API_EVENT_WAVEBAND 1
//#define API_EVENT_VOLUME   2

void handleApiEventTone(api_event_t event, uint32_t param, char *s, bool isDebug = false) {
uint16_t pressValue = param & 0xffff;
uint16_t repeatCount = param >> 16;
static bool direct = false;
enum state_tone_t {
  STATE_TONE_IDLE = 0,
  STATE_TONE_DO_MUTE,
  STATE_TONE_DONE
};
static state_tone_t stateTone = STATE_TONE_IDLE;
strcpy(s, "");
  switch (stateTone) {
    case STATE_TONE_IDLE:
      direct = pressValue > TOUCHREAD_DIRECTDELTA;
      if (API_EVENT_TONE_TOUCH == event) {
        if (apiLock) 
          toggleMedionLED(LED_MODE_DARK);
        else
          if (muteflag) {
            muteflag = false;
            stateTone = STATE_TONE_DONE;  
            strcpy(s, "Undo mute!");
          }
      } else if (apiLock) {
        if (API_EVENT_TONE_UNTOUCH == event) {
          toggleMedionLED(LED_MODE_FORCELOW);
          strcpy(s, "API is locked!");
        }
        else if ((API_EVENT_TONE_LONGPRESSED == event) && direct && (5 == repeatCount)) {
          apiLock = false;
          sprintf(s, "API unlocked!");
          toggleMedionLED(LED_MODE_FORCELOW);
          toggleMedionLED(LED_MODE_FLASH2);
          stateTone = STATE_TONE_DONE;   
        }  
      }
      else if (API_EVENT_TONE_UNTOUCH != event) {
          if (direct) {
            if (API_EVENT_TONE_PRESSED == event) {
              if (localfile) 
                selectPreset(0xff, isDebug);
              else if (SD_nodecount > 0) {
                analyzeCmd("mp3track", "0");
              }
            }
            if ((API_EVENT_TONE_3PRESSED == event)) {
              analyzeCmd("mute");  
              stateTone = STATE_TONE_DONE;
              strcpy(s, "Starting mute!");
            } else if (API_EVENT_TONE_2PRESSED == event) {
              if (localfile) {
                analyzeCmd("mp3track", "0");
                stateTone = STATE_TONE_DONE;
                strcpy(s, "Random play something else from SD!");
              } else {
                selectRandomPreset();
                stateTone = STATE_TONE_DONE;
                strcpy(s, "Random play one of the known presets!");                
              }
              
            } else if (API_EVENT_TONE_LONGPRESSED == event) {
              analyzeCmd("downvolume", "2");
              if ((ini_block.reqvol < MINVOLUME) && (ini_block.reqvol > 0))
                analyzeCmd("volume", "0");
            } else if (API_EVENT_TONE_2LONGPRESSED == event) {
              analyzeCmd("upvolume", "2");
              if (ini_block.reqvol < MINVOLUME) {
                char s[30];
                sprintf(s, "%d", MINVOLUME);
                analyzeCmd("volume", s);
              }
            }
          } else {
            if (API_EVENT_TONE_LONGPRESSED == event) {
              Serial.printf("LongPressed on Gehäuse, param=%ld\r\n", repeatCount);
              
              if (repeatCount == 0)
                selectPreset(waveBand * NUMCHANNELS, isDebug);
            }
            if (API_EVENT_TONE_PRESSED == event) {
              if ( equalizers.size() > 0 ) 
                setEqualizer((currentEqualizer + 1 ) % equalizers.size());
              Serial.println("Pressed on Gehäuse");
            }
            else if (API_EVENT_TONE_2PRESSED == event)
              Serial.println("2Pressed on Gehäuse");
            else if (API_EVENT_TONE_3PRESSED == event) {
              apiLock = !apiLock;
//              toggleMedionLED(LED_MODE_FORCELOW);
              toggleMedionLED(LED_MODE_FLASH2);
              Serial.printf("3Pressed on Gehäuse, apiLock=%d\r\n", apiLock);
            }
            else if (API_EVENT_TONE_3LONGPRESSED == event) {
              if (repeatCount == 0) {
//                toggleMedionLED(LED_MODE_FORCELOW);
                toggleMedionLED(LED_MODE_FLASH1);
                apiVolumeLock = !apiVolumeLock;
                Serial.printf("3longPressed on Gehäuse, apiVolumeLock=%d\r\n", apiVolumeLock);
              }
              
            }
          }
        } else {      //Touch release (untouch)

        }
      break;
    case STATE_TONE_DONE:
      if (API_EVENT_TONE_UNTOUCH == event)
        stateTone = STATE_TONE_IDLE;
      break;
  }
}




void handleApiEvent(api_event_t event, uint32_t param, bool init = false) {
char s[500];  
bool debuginfo;
bool switchChannel = false;
bool isTouchEvent;
#if defined(MEDION_DEBUGAPI)
  debuginfo = true;  
#else
  debuginfo = false;  
#endif
  if (event == API_EVENT_CHANNEL) {
    sprintf(s, "API event channel: %04X (from %d to %d)", param, (param & 0xff00) / 0x100, param & 0xff);        
    tunedChannel = param & 0xff;
    if (apiLock)
      strcpy(s, "API is locked!");
    else if (localfile && (waveBand == medionConfig.mp3Waveband)) {
      switchChannel = false;
      analyzeCmd("mp3track", "0");
    } else
      switchChannel = !init;
  } else if (event == API_EVENT_WAVEBAND) {
    sprintf(s, "API event waveband: %ld", param); 
    Serial.printf("WaveBand = %d\r\n", waveBand);
    if (apiLock)
      strcpy(s, "API is locked!");
    else if (SD_okay && (0 < SD_nodecount) && ((param & 0xff) == medionConfig.mp3Waveband)) {
      switchChannel = false;
      toggleMedionLED(LED_MODE_LOW);
      analyzeCmd("mp3track", "0");  
    } else {
      switchChannel = !init;
    }
  } else if (event == API_EVENT_VOLUME) {
    char s1[40];
    sprintf(s1, "volume=%ld", param);
    sprintf(s, "API event volume: %ld", param);
    if (apiLock) {
      ;//strcpy(s, "API is locked!");
    } else if (apiVolumeLock)
      ;//strcpy(s, "API volume is locked!");
    else if (!muteflag) {
        analyzeCmd(s1);
    }
    else {
      strcat(s, " (ignored since muteflag is set!)");
      muteflag = (param != 0);
    }
  } else if (isTouchEvent = (API_EVENT_TONE_TOUCH == event) ||
             (API_EVENT_TONE_UNTOUCH == event) ||
             (API_EVENT_TONE_PRESSED == event) ||
             (API_EVENT_TONE_LONGPRESSED == event) ||
             (API_EVENT_TONE_LONGRELEASED == event) ||
             (API_EVENT_TONE_2PRESSED == event) ||
             (API_EVENT_TONE_2LONGPRESSED == event) ||
             (API_EVENT_TONE_2LONGRELEASED  == event) ||
             (API_EVENT_TONE_3PRESSED == event) ||
             (API_EVENT_TONE_3LONGPRESSED == event) ||
             (API_EVENT_TONE_3LONGRELEASED  == event)
             
             ) 
      handleApiEventTone(event, param, s, debuginfo);//, s, debuginfo);
  else
    sprintf(s, "Unknown API event: %d with param: %ld\r\n", event, param);
  if ((strstr(s, "API") == s) && (NULL != strstr(s, "is locked!")) && !isTouchEvent) 
    toggleMedionLED(LED_MODE_FLASH3);
  if (debuginfo)
    if (strlen(s))
      dbgprint(s);
  if (switchChannel) {
    selectPreset(tunedChannel + waveBand * NUMCHANNELS, debuginfo);
    /*
    uint16_t channel = tunedChannel + waveBand * NUMCHANNELS;
    toggleMedionLED(HIGH);
    if (debuginfo)
      Serial.printf("API switch to channel: %d\r\n", channel);
    sprintf(s, "preset=%d", channel);
    analyzeCmd(s);
    */
  }
}

void setupMedionExtension() {
nvs_handle nvsHandler;
  if (medionExtensionSetupDone)
    return;

  //analyzeCmd("debug", "0");
  
  medionExtensionSetupDone = true;
  if (ESP_OK == nvs_open("MEDION80806", NVS_READWRITE, &nvsHandler)) {
    size_t len;
    if (NVS_ERASE)
      nvs_erase_all(nvsHandler);
    len = sizeof(medionPins);
    if (ESP_OK == nvs_get_blob(nvsHandler, "pins", NULL, &len))
      nvs_get_blob(nvsHandler, "pins", &medionPins, &len);
    else
      {
        nvs_set_blob(nvsHandler, "pins", &medionPins, sizeof(medionPins));          
      }
    len = sizeof(medionConfig);
    if (ESP_OK == nvs_get_blob(nvsHandler, "config", NULL, &len)) {
      nvs_get_blob(nvsHandler, "config", &medionConfig, &len);
      len = medionConfig.waveSwitchPositions * sizeof(uint16_t) * 2;
      if (medionSwitchPositionCalibration = (uint16_t *)malloc(len))
        nvs_get_blob(nvsHandler, "switch", medionSwitchPositionCalibration, &len);
      len = medionConfig.numChannels * sizeof(uint16_t) * 2;
      if (medionChannelLimitsNew = (uint16_t *)malloc(len))
        nvs_get_blob(nvsHandler, "capa", medionChannelLimitsNew, &len);
      } 
    else
      {
      nvs_set_blob(nvsHandler, "config", &medionConfig, sizeof(medionConfig));  
      len = medionConfig.waveSwitchPositions * sizeof(uint16_t) * 2;
      nvs_set_blob(nvsHandler, "switch", &medionSwitchPositionCalibrationDefault, len);
      len = medionConfig.numChannels * sizeof(uint16_t) * 2;
      nvs_set_blob(nvsHandler, "capa", medionChannelLimitsNew, len);

      }
    nvs_commit(nvsHandler);  
  }
  touch_pad_init();
  touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
  touch_pad_filter_start(10);  
  if (medionPins.led != 0xff) {
    ledcAttachPin(medionPins.led, 0); // Attach LED to Driver channel 0
    ledcSetup(0, 4000, 8);
    toggleMedionLED(LED_MODE_LOW);
  }
  
  if (medionPins.capaPin != TOUCH_PAD_MAX)
    touch_pad_config(medionPins.capaPin, 0);
  if (medionPins.tonePin != TOUCH_PAD_MAX)
    touch_pad_config(medionPins.tonePin, 0);
  if (medionPins.vol != 0xff)
    pinMode(medionPins.vol, INPUT);
  if (medionPins.wave != 0xff)
    pinMode(medionPins.wave, INPUT);
    //capaReadPin = new VirtualTouchPin(medionPins.capaPin);

   volumeRead();
   readChannel(); 
   toneRead();  
   readPresetList();
   
}


void handleMedionLoop() {
static bool first = true;
static bool statusWasNeverPublished = true;
    if (first)
      setupMedionExtension();
    if (first || (1 == playingstat) || (localfile))
      {
        toggleMedionLED(LED_MODE_LOW);
        first = false;
      }
   volumeRead();
//   if ((1 == playingstat) && (0 != lastVolume))   
      readChannel(); 
   toneRead();    
}


struct brightness_step_t {
  uint8_t width;
  uint16_t brightness;
};



void *getLowTable(size_t& tableSize) {
/*
static brightness_step_t defaultLowTable[] = {{15, 0xf00}, 
                                              {2, 0x1000}, {1, 0x2000}, {1, 0x3000},{2, 0x4000}, 
                                              {3, 0x5000}, 
                                              {2, 0x4000}, {1, 0x3000}, {1, 0x2000},{2, 0x1000}
                                             }; 
*/
static brightness_step_t defaultLowTable[] = {{40, 0xf00}, 
                                              {1, 0x1c00},
                                              {1, 0x2800},
                                              {1, 0x4000}, 
                                              {1, 0x6000}, 
                                              {1, 0x9000}, 
                                              {1, 0x6000},
                                              {1, 0x4000},
                                              {1, 0x2800},
                                              {1, 0x1c00}                                               
                                             }; 

static brightness_step_t mp3playdefaultLowTable[] = {{40, 0xf00}, 
                                              {1, 0x1c00},
                                              {1, 0x2800},
                                              {1, 0x4000}, 
                                              {1, 0x6000}, 
                                              {1, 0x9000}, 
                                              {1, 0x6000},
                                              {1, 0x4000},
                                              {1, 0x2800},
                                              {1, 0x1c00},                                               
                                              {1, 0x2800},
                                              {1, 0x4000}, 
                                              {1, 0x6000}, 
                                              {1, 0x9000}, 
                                              {1, 0x6000},
                                              {1, 0x4000},
                                              {1, 0x2800},
                                              {1, 0x1c00}                                               
                                             }; 


static brightness_step_t apiLockLowTable[] = {{40, 0x500},
                                              {3, 0x400},
                                              {1, 0x300}, 
                                              {1, 0x200}, {1, 0x100},
                                              {1, 0x0}, 
                                              {1, 0x100}, {1, 0x200},
                                              {1, 0x300},
                                              {3, 0x400}                                               
                                             }; 

static brightness_step_t mp3playapiLockLowTable[] = {{40, 0x500},
                                              {1, 0x400},
                                              {1, 0x300}, 
                                              {1, 0x200}, {1, 0x100},
                                              {1, 0x0}, 
                                              {1, 0x100}, {1, 0x200},
                                              {1, 0x300},
                                              {1, 0x400},                                               
                                              {1, 0x300}, 
                                              {1, 0x200}, {1, 0x100},
                                              {1, 0x0}, 
                                              {1, 0x100}, {1, 0x200},
                                              {1, 0x300},
                                              {1, 0x400}                                               
                                             }; 

static brightness_step_t volLockLowTable[] = {{40, 0xf00}, 
                                              {1, 0x1000}, {1, 0x1800}, {1, 0x2000}, 
                                              {1, 0x6000}, 
                                              {1, 0x2000}, {1, 0x1800}, {1, 0x1000},
                                              {1, 0xf00},
                                              {1, 0x400},
                                              {1, 0x000},
                                              {1, 0x400}
                                             }; 

static brightness_step_t mp3playvolLockLowTable[] = {{40, 0xf00}, 
                                              {1, 0x1000}, {1, 0x1800}, {1, 0x2000}, 
                                              {1, 0x6000}, 
                                              {1, 0x2000}, {1, 0x1800}, {1, 0x1000},
                                              {1, 0x1800}, {1, 0x2000}, 
                                              {1, 0x6000}, 
                                              {1, 0x2000}, {1, 0x1800}, {1, 0x1000},
                                              {1, 0xf00},
                                              {1, 0x400},
                                              {1, 0x000},
                                              {1, 0x400}
                                             }; 


/*                                             
static brightness_step_t volLockLowTable[]  = {{15, 0xf00}, 
                                              {1, 0x2000}, {1, 0x4000}, 
                                              {2, 0x5000}, 
                                              {1, 0x4000},  {1, 0x2000},
                                              {1, 0xf00}, 
                                              {1, 0xa00}, {1, 0x400}, 
                                              {2, 0x00}, 
                                              {1, 0x400},  {1, 0xa00}
                                             };
*/
brightness_step_t *ret;
  if (localfile) {
    if (apiLock) {
      ret = mp3playapiLockLowTable;
      tableSize = sizeof(mp3playapiLockLowTable) / sizeof(mp3playapiLockLowTable[0]);    
    } else if (apiVolumeLock) {
      ret = mp3playvolLockLowTable;
      tableSize = sizeof(mp3playvolLockLowTable) / sizeof(mp3playvolLockLowTable[0]);    
    } else {
      ret = mp3playdefaultLowTable;
      tableSize = sizeof(mp3playdefaultLowTable) / sizeof(mp3playdefaultLowTable[0]);
    }
  } else {
    if (apiLock) {
      ret = apiLockLowTable;
      tableSize = sizeof(apiLockLowTable) / sizeof(apiLockLowTable[0]);    
    } else if (apiVolumeLock) {
      ret = volLockLowTable;
      tableSize = sizeof(volLockLowTable) / sizeof(volLockLowTable[0]);    
    } else {
      ret = defaultLowTable;
      tableSize = sizeof(defaultLowTable) / sizeof(defaultLowTable[0]);
    }
  }
  return ret;
}

void toggleMedionLED(led_mode_t ledMode) {
enum led_state_t {
  LED_STATE_LOW = 0,
  LED_STATE_HIGH, 
  LED_STATE_INIT,
  LED_STATE_FLASH = 100
}  lastState;

static led_state_t ledState = LED_STATE_INIT;

static brightness_step_t defaultHighTable[] = {{1, 0xffff}}; 
static brightness_step_t flash3Table[] = {{1, 0xffff},{2, 0},{2, 0xffff},{3, 0}, {6,0xffff}};
static brightness_step_t flash2Table[] = {{2, 0xffff},{2, 0},{8, 0xffff},{3, 0}};
static brightness_step_t flash1Table[] = {{1, 0xffff},{1, 0},{10, 0xffff}};
static brightness_step_t darkTable[] = {{5000, 0}};

static brightness_step_t *brightnessTable = NULL;
static size_t brightnessTableSize = 0;
static size_t brightnessStep;
static uint8_t brightnessStepIdx;
brightness_step_t *p;

static uint16_t brightness = 0;
static uint32_t stateTime = 0;
bool changed;
  if (medionPins.led == 0xff)
    return;
  changed = false;
  lastState = ledState;
  switch (ledMode) {
    case LED_MODE_HIGH:
      ledState = LED_STATE_HIGH;
      brightnessTable = defaultHighTable;
      brightnessTableSize = 1;
      break;
    case LED_MODE_LOW:
      if (ledState < 100) {
        p = (brightness_step_t *)getLowTable(brightnessTableSize);
        ledState = LED_STATE_LOW;
        if (p != brightnessTable) {
          lastState = LED_STATE_HIGH;
          brightnessTable = p;
        }
      }
      break;
    case LED_MODE_FORCELOW:
        p = (brightness_step_t *)getLowTable(brightnessTableSize);
        ledState = LED_STATE_LOW;
        if (p != brightnessTable) {
          lastState = LED_STATE_HIGH;
          brightnessTable = p;
        }
      break;
    case LED_MODE_FLASH3:
      if (ledState < 100) {
        ledState = LED_STATE_FLASH;
        brightnessTable = flash3Table;
        brightnessTableSize = sizeof(flash3Table) / sizeof(flash3Table[0]);
      }
      break;
    case LED_MODE_FLASH2:
      if (ledState < 100) {
        ledState = LED_STATE_FLASH;
        brightnessTable = flash2Table;
        brightnessTableSize = sizeof(flash2Table) / sizeof(flash2Table[0]);
      }
      break;    
    case LED_MODE_FLASH1:
      if (ledState < 100) {
        ledState = LED_STATE_FLASH;
        brightnessTable = flash1Table;
        brightnessTableSize = sizeof(flash1Table) / sizeof(flash1Table[0]);
      }
      break;
    case LED_MODE_DARK:
      ledState = LED_STATE_FLASH;
      brightnessTable = darkTable;
      brightnessTableSize = sizeof(darkTable) / sizeof(darkTable[0]);
      break;
    case LED_MODE_TOGGLE:
        if ((LED_STATE_LOW == ledState) || (LED_STATE_INIT == ledState)) {
          ledState = LED_STATE_HIGH;
          brightnessTable = defaultHighTable;
          brightnessTableSize = sizeof(defaultHighTable);
        }
        else if (LED_STATE_HIGH == ledState) {
          ledState = LED_STATE_LOW;
          brightnessTable = (brightness_step_t *)getLowTable(brightnessTableSize);
        }
      break;
  }
  if ((NULL == brightnessTable) || (0 == brightnessTableSize)) {
    brightness = 0;
    changed = true;
  } else {
    if (changed = (lastState != ledState)) {
//      Serial.printf("LED state changed! to %d\r\n", ledState);
      stateTime = millis();
      brightnessStep = 0;
      brightnessStepIdx = 0;
      brightness = brightnessTable[0].brightness;
    } else {
      if (changed = (millis() - stateTime > 50)) {
        stateTime = millis();
        if (++brightnessStepIdx >= brightnessTable[brightnessStep].width) {
          brightnessStepIdx = 0;
          brightnessStep++;
          if (brightnessTableSize == brightnessStep) {
            brightnessStep = 0;
            if (ledState >= 100) {
              ledState = LED_STATE_LOW;
              brightnessTable = (brightness_step_t*)getLowTable(brightnessTableSize);
            }
          }
        }
        brightness = brightnessTable[brightnessStep].brightness;
      }
    }
  }
  if (changed) {
    uint16_t x;
    if (medionPins.ledInverted)
      x = ~brightness;
    else
      x = brightness;
    static uint32_t lastInfo = 0;
/*
    if (millis() - lastInfo > 2000) {
      Serial.printf("table[%d].width=%d (idx = %d), table[*].bri=0x%04x \r\n", 
          brightnessStep, brightnessTable[brightnessStep].width, brightnessStepIdx, 
          brightnessTable[brightnessStep].brightness);
      lastInfo = millis();
    }
    */
    ledcWrite(0, x >> 8);
  }
}

void volumeRead() {
static int32_t lastVolume = 0;
static uint8_t lastWaveBand = 0xff; 
static uint16_t waveBandCount = 0;
static uint32_t lastRead = 0;
static uint16_t ticks = 0;
static uint16_t count = 0;
static int32_t total = 0;
static uint32_t totalWave = 0;
static uint32_t waveCount = 0;

uint16_t currentRead = 0;

static bool init = true;
  if (medionPins.vol != 0xff)
    {
    currentRead = analogRead(medionPins.vol);
//    if (currentRead < 4000)
      {
        count++;
        total = total + currentRead;    
      }
    }  
  if (medionPins.wave != 0xff) {
    uint16_t waveRead = analogRead(medionPins.wave);
    totalWave += waveRead;
    waveCount++;
    
  }
    if ((++ticks >= (localfile?10:100)) || init) {
      if (count > 0) 
        {
#ifdef MEDIONCALIBRATE_VOLUME
        static uint32_t lastReportTime = 0;
        if (millis() > lastReportTime + 2000) {
          Serial.printf("VolumeReadPhys: %ld\r\n", total / count);
          lastReportTime = millis();
        }
#endif
          total = map(total / count, 0, 4096, MINVOLUME, MAXVOLUME);
          if (total > MAXVOLUME)
            total = MAXVOLUME;
          count = 0;
          if ((abs(lastVolume-total) > 1) || init || ((total != lastVolume) && ((total == 0) || (total == 100)))) // Noisy potentiometer?
            {
              Serial.printf("VolumeDelta: %d\r\n", abs(lastVolume-total));
              lastVolume = total;
              handleApiEvent(API_EVENT_VOLUME, total, init);         
            }
          }
     if ((waveCount != 0))
      {
//        uint32_t waveRead = totalWave / ticks;
//        uint16_t waveRead = analogRead(medionPins.wave);
//        uint32_t waveRead = analogRead(medionPins.wave);
        uint8_t waveBandNew;          
        totalWave = totalWave / waveCount;
        for(waveBandNew = MEDION_WAVE_SWITCH_POSITIONS-1;waveBandNew < 0xff;waveBandNew--)
        if ((totalWave >= medionSwitchPositionCalibration[waveBandNew * 2]) &&
            (totalWave <= medionSwitchPositionCalibration[waveBandNew * 2 + 1]))
               break;
        
#if defined(MEDIONCALIBRATE_WAVEBAND)
        static uint32_t lastReportTime = 0;
        if (millis() > lastReportTime + 2000) {
          if (DEBUG)
            dbgprint("WaveReadPhys: %ld, detected Position: %d, total reads=%ld", totalWave, waveBandNew, waveCount);
          else
            Serial.printf("WaveReadPhys: %ld, detected Position: %d, total reads=%ld\r\n", totalWave, waveBandNew, waveCount);
          lastReportTime = millis();
        }
#endif               
        if ((waveBandNew != lastWaveBand) && (waveBandNew != 0xff)) 
          {
          if (waveBandNew == 255)
            Serial.printf("Hier mit waveBandnew == 255???????????????????????????????????\r\n");
            
#if defined(MEDIONCALIBRATE_WAVEBAND)
          if (DEBUG)
            dbgprint("NEW WaveBand: %d (war: %d) (phys: %ld), valid reads=%ld", waveBandNew, lastWaveBand, totalWave, waveCount);
          else
            Serial.printf("NEW WaveBand: %d (war: %d) (phys: %ld), valid reads=%ld\r\n", waveBandNew, lastWaveBand, totalWave, waveCount);
          for(int i = 0;i < MEDION_WAVE_SWITCH_POSITIONS;i++) 
            Serial.printf("[%4d, %4d] ", medionSwitchPositionCalibration[i*2], medionSwitchPositionCalibration[i*2 + 1]);
          Serial.println();
#endif
          waveBand = lastWaveBand = waveBandNew;  
          handleApiEvent(API_EVENT_WAVEBAND, waveBand, init); 
          }
     }
     totalWave = 0;
     waveCount = 0;
     waveBandCount = 0;      
     total = 0;
     ticks = 0;        
    }
  init = false;
}
  
void readChannel() {
    static uint8_t lastReportedChannel = 0xff;
    static uint32_t lastCapaReadTime = 0;
    uint8_t channel;
    bool hit = false;
//    bool timeToReport = false;
    uint16_t * medionChannelLimits = medionChannelLimitsNew;
    uint16_t capRead;
    enum capa_read_state_t {
      CAPAREAD_STATE_INIT = 0,
      CAPAREAD_STATE_READ = 1,
      CAPAREAD_STATE_WAIT = 2
      
    }
    static capa_read_state = CAPAREAD_STATE_INIT;

    switch (capa_read_state) {
      case CAPAREAD_STATE_INIT:
        if (TOUCH_PAD_MAX != medionPins.capaPin)
          capa_read_state = CAPAREAD_STATE_READ;
        break;
      case CAPAREAD_STATE_READ:
        touch_pad_read_filtered(medionPins.capaPin, &capRead);
#if !defined(MEDIONCALIBRATE_CAPACITOR)
        if ((capRead >= medionChannelLimits[0]) || (capRead <= medionChannelLimits[NUMCHANNELS * 2 - 1])) {
#endif
    channel = 0;
    while (!hit && (channel < NUMCHANNELS) && (capRead >= medionChannelLimits[channel*2])) {
      hit = (capRead <= medionChannelLimits[channel * 2 + 1]);
      if (!hit)
        channel++;
    }
    if (!hit && (0xff ==lastReportedChannel)) {
      hit = true;
      if (NUMCHANNELS == channel)
        channel--;
    }

#if defined(MEDIONCALIBRATE_CAPACITOR)
  static uint16_t lastCapRead = 0;  
  static uint32_t lastReportTime = 0;
  if (millis() > lastReportTime + 2000) {
    if (lastCapRead == 0)
      lastCapRead = capRead;
    lastReportTime = millis();      
//    timeToReport = true;
    dbgprint("CapRead= %d (changed: %d), isHit(chan[%d (of %d)]) = %d", capRead, (capRead != lastCapRead), channel, NUMCHANNELS, hit);
  }
  lastCapRead = capRead;
#endif

    if (hit && (lastReportedChannel != channel)) {
      uint32_t channelparam = ((uint32_t)lastReportedChannel) * 256 + channel;
      handleApiEvent(API_EVENT_CHANNEL, channelparam, lastReportedChannel == 0xff);         
      //Serial.printf("Set lastreportetchannel= %02x (was %02x)\r\n", channel, lastReportedChannel);
      lastReportedChannel = channel;
    }


#if !defined(MEDIONCALIBRATE_CAPACITOR)
        }
#endif
      lastCapaReadTime = millis();
      capa_read_state = CAPAREAD_STATE_WAIT;
    break;
    case CAPAREAD_STATE_WAIT:
      if (millis() - lastCapaReadTime > 100) {
        capa_read_state = CAPAREAD_STATE_READ;
      }
      break;
    default:
      // should not happen!
      break;
    }

}


void toneRead() {
  enum tone_read_state_t {
      TONEREAD_STATE_INIT = 0,
      TONEREAD_STATE_IDLE,
      TONEREAD_STATE_DEBOUNCE,
      TONEREAD_STATE_PRESSED,
      TONEREAD_STATE_2PRESSED,
      TONEREAD_STATE_3PRESSED,
      TONEREAD_STATE_PRESSED_DONE,
      TONEREAD_STATE_2PRESSED_DONE,
      TONEREAD_STATE_3PRESSED_DONE,
      TONEREAD_STATE_LONGIDLE,
      TONEREAD_STATE_2LONGIDLE,
      TONEREAD_STATE_3LONGIDLE,
      
  };
  static tone_read_state_t toneReadState = TONEREAD_STATE_INIT;
  uint16_t toneRead;
  static uint16_t toneReadMin = 0xffff;
  static uint32_t stateStartTime;
  static uint32_t longRepeats = 0;
  static uint16_t touchLevelUnpressed = 0;
  if (TONEREAD_STATE_INIT != toneReadState) {
    touch_pad_read_filtered(medionPins.tonePin, &toneRead);
    if (toneReadMin > toneRead)
      toneReadMin = toneRead;
#if defined(MEDIONCALIBRATE_TONE)
    static uint32_t lastReportTime = 0;
    if (millis() > lastReportTime + 2000) {
      lastReportTime = millis();      
      dbgprint("ToneRead= %d (untouched level: %d)", toneRead, touchLevelUnpressed);
    }
#endif
  }
  switch (toneReadState) {
    case TONEREAD_STATE_INIT:
      if (TOUCH_PAD_MAX != medionPins.tonePin) {
        toneReadState = TONEREAD_STATE_IDLE;
      }
      break;  
    case TONEREAD_STATE_IDLE:
//        if (toneRead < TOUCHLEVEL_TONE) {
        if (toneRead < touchLevelUnpressed - TOUCHREAD_MINDELTA) {  
          stateStartTime = millis();
          toneReadState = TONEREAD_STATE_DEBOUNCE;
          toneReadMin = toneRead;
        } else {
          static uint32_t lastTestTime = 0;
          if (toneRead > touchLevelUnpressed) {
            touchLevelUnpressed = toneRead;
            lastTestTime = millis();
          } else if (millis() - lastTestTime > 500) {
            touchLevelUnpressed--;
          }
        }
      break;
    case TONEREAD_STATE_DEBOUNCE:
        if (toneRead > touchLevelUnpressed - TOUCHREAD_MINDELTA) {
          toneReadState = TONEREAD_STATE_IDLE;
        } else if (millis() - stateStartTime > 50) {
          stateStartTime = millis();
          toneReadState = TONEREAD_STATE_PRESSED;
          handleApiEvent(API_EVENT_TONE_TOUCH, touchLevelUnpressed - toneReadMin);
        }
      break;
    case TONEREAD_STATE_PRESSED:
        if (toneRead > touchLevelUnpressed - TOUCHREAD_MINDELTA) {
          stateStartTime = millis();
          toneReadState = TONEREAD_STATE_PRESSED_DONE;
        } else if (millis() - stateStartTime > TOUCHTIME_TONELONG1) {
          toneReadState = TONEREAD_STATE_LONGIDLE;
          handleApiEvent(API_EVENT_TONE_LONGPRESSED, touchLevelUnpressed - toneReadMin);
          longRepeats = 0;
          stateStartTime = millis();
        }
      break;  
    case TONEREAD_STATE_PRESSED_DONE:
        if (millis() - stateStartTime > TOUCHTIME_TONE2PRESS) {        
            handleApiEvent(API_EVENT_TONE_PRESSED, touchLevelUnpressed - toneReadMin);
            handleApiEvent(API_EVENT_TONE_UNTOUCH, touchLevelUnpressed - toneReadMin);
            toneReadState = TONEREAD_STATE_IDLE;
        } else if (millis() - stateStartTime > 50) {
          if (toneRead < touchLevelUnpressed - TOUCHREAD_MINDELTA) {  
            stateStartTime = millis();
            toneReadState = TONEREAD_STATE_2PRESSED;
          }          
        }
      break;
    case TONEREAD_STATE_2PRESSED:
        if (toneRead > touchLevelUnpressed - TOUCHREAD_MINDELTA) {
          stateStartTime = millis();
          toneReadState = TONEREAD_STATE_2PRESSED_DONE;
        } else if (millis() - stateStartTime > TOUCHTIME_TONELONG1) {
          toneReadState = TONEREAD_STATE_2LONGIDLE;
          handleApiEvent(API_EVENT_TONE_2LONGPRESSED, touchLevelUnpressed - toneReadMin);
          longRepeats = 0;
          stateStartTime = millis();
        }
      break;  
    case TONEREAD_STATE_2PRESSED_DONE:
        if (millis() - stateStartTime > TOUCHTIME_TONE2PRESS) {        
            handleApiEvent(API_EVENT_TONE_2PRESSED, touchLevelUnpressed - toneReadMin);
            handleApiEvent(API_EVENT_TONE_UNTOUCH, touchLevelUnpressed - toneReadMin);
            toneReadState = TONEREAD_STATE_IDLE;
        } else if (millis() - stateStartTime > 50) {
          if (toneRead < touchLevelUnpressed - TOUCHREAD_MINDELTA) {  
            stateStartTime = millis();
            toneReadState = TONEREAD_STATE_3PRESSED;
          }          
        }
      break;
    case TONEREAD_STATE_3PRESSED:
        if (toneRead > touchLevelUnpressed - TOUCHREAD_MINDELTA) {
          stateStartTime = millis();
          toneReadState = TONEREAD_STATE_3PRESSED_DONE;
        } else if (millis() - stateStartTime > TOUCHTIME_TONELONG1) {
          toneReadState = TONEREAD_STATE_3LONGIDLE;
          handleApiEvent(API_EVENT_TONE_3LONGPRESSED, touchLevelUnpressed - toneReadMin);
          longRepeats = 0;
          stateStartTime = millis();
        }
      break;  
    case TONEREAD_STATE_3PRESSED_DONE:
        if (millis() - stateStartTime > TOUCHTIME_TONE2PRESS) {        
            handleApiEvent(API_EVENT_TONE_3PRESSED, touchLevelUnpressed - toneReadMin);
            handleApiEvent(API_EVENT_TONE_UNTOUCH, touchLevelUnpressed - toneReadMin);
            toneReadState = TONEREAD_STATE_IDLE;
        } else if (millis() - stateStartTime > 50) {
          if (toneRead < touchLevelUnpressed - TOUCHREAD_MINDELTA) {  
            stateStartTime = millis();
            toneReadState = TONEREAD_STATE_3PRESSED;
          }          
        }
      break;

    case TONEREAD_STATE_LONGIDLE:
        if (toneRead > touchLevelUnpressed - TOUCHREAD_MINDELTA) {
          handleApiEvent(API_EVENT_TONE_LONGRELEASED, longRepeats + touchLevelUnpressed - toneReadMin);
          handleApiEvent(API_EVENT_TONE_UNTOUCH, touchLevelUnpressed - toneReadMin);
          toneReadState = TONEREAD_STATE_IDLE;
        } else if (millis() - stateStartTime > TOUCHTIME_TONELONGFF) {
          longRepeats = longRepeats + 0x10000;
          handleApiEvent(API_EVENT_TONE_LONGPRESSED, longRepeats + touchLevelUnpressed - toneReadMin);
          stateStartTime = millis();          
        }
      break;  
    case TONEREAD_STATE_2LONGIDLE:
        if (toneRead > touchLevelUnpressed - TOUCHREAD_MINDELTA) {
          handleApiEvent(API_EVENT_TONE_2LONGRELEASED, longRepeats + touchLevelUnpressed - toneReadMin);
          handleApiEvent(API_EVENT_TONE_UNTOUCH, touchLevelUnpressed - toneReadMin);
          toneReadState = TONEREAD_STATE_IDLE;
        } else if (millis() - stateStartTime > TOUCHTIME_TONELONGFF) {
          longRepeats = longRepeats + 0x10000;
          handleApiEvent(API_EVENT_TONE_2LONGPRESSED, longRepeats + touchLevelUnpressed - toneReadMin);
          stateStartTime = millis();          
        }
      break;  
    case TONEREAD_STATE_3LONGIDLE:
        if (toneRead > touchLevelUnpressed - TOUCHREAD_MINDELTA) {
          handleApiEvent(API_EVENT_TONE_3LONGRELEASED, longRepeats + touchLevelUnpressed - toneReadMin);
          handleApiEvent(API_EVENT_TONE_UNTOUCH, touchLevelUnpressed - toneReadMin);
          toneReadState = TONEREAD_STATE_IDLE;
        } else if (millis() - stateStartTime > TOUCHTIME_TONELONGFF) {
          longRepeats = longRepeats + 0x10000;
          handleApiEvent(API_EVENT_TONE_3LONGPRESSED, longRepeats + touchLevelUnpressed - toneReadMin);
          stateStartTime = millis();          
        }
      break;  
    default:
      break;
  }

}
