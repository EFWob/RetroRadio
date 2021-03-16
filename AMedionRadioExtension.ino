#ifdef OLD
#include <nvs.h>
#include "AMedionRadioExtension.h"
#include "ARetroRadioExtension.h"

//RetroRadioInput *dummy = NULL;//("a33");
//RetroRadioInputADC *dummyVol = NULL;//("a33");
//RetroRadioInputADC *dummySwitch = NULL;//("a35")

#define NVS_ERASE     true            // NVS will be completely deleted if set to true (only use for development, must be false in field SW)
#define NUMCHANNELS   15
//#define NUMWAVEBANDS   3
#define MP3WAVEBAND    255
//#define MINVOLUME       50
//#define MAXVOLUME      100


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
//static uint8_t tunedChannel = 0;
static bool hmiLock = false;
static bool hmiVolumeLock = false;


static uint8_t switchPosition = 0xff;
static int8_t volumePosition = -1;
std::vector<int16_t> switchReadings;
std::vector<int16_t> tuneReadings[10]; 
std::vector<int16_t> ledPattern[10];
uint8_t numLeds = 0;
uint8_t useTuneReadings = 0;
int16_t tunedChannel = -1;
int16_t lastTunedChannel = -1;
int16_t lastSetChannel = -1;
int16_t nearestChannel = -1;
int16_t physTuneReading = -1;
touch_pad_t tunePin = TOUCH_PAD_MAX;

String randMode("");

void evaluateTuning();



std::vector<int16_t>& getTuneReadings(uint8_t idx) {
  if (idx >= 10)
    return tuneReadings[0];
  if (tuneReadings[idx].size() == 0)
    return tuneReadings[0];
  return tuneReadings[idx];
}

void useTunings(uint8_t idx) {
  if (idx >= 10)
    idx = 10;
  tunedChannel = lastTunedChannel =  lastSetChannel = nearestChannel = -1;
  useTuneReadings = idx;
//  evaluateTuning();
}

void appendPairList(std::vector<int16_t>& v, String value) {
  char *s = strdup(value.c_str());
  if (s) {
    char *first = s;
    while (first) {
      char * second = strchr(first, ',');
      if (NULL == second) 
        first = NULL;
      else {                  // we expect a comma delimited pair of integers.
        *second = 0;
        second++;
        char *next = strchr(second, ',');
        if (next) {
          *next = 0;
          next++;
        }
//        Serial.printf("PushBack %d\r\n", atoi(first));
        int ifirst = atoi(first);
        int isecond = atoi(second);
        if (isecond < ifirst) {
          int t = ifirst;
          ifirst = isecond;
          isecond = t;
        }
        v.push_back(ifirst);
        v.push_back(isecond);
        first = next;
      }
     
    }
    free(s);
  }
}

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

void executeCmdsold(String commands, String value) {
//    Serial.printf("EXECUTE: %s, value=$(%s)\r\n", commands.c_str(), value.c_str());
    bool haveValue = value.length() > 0;
    char *s = strdup(commands.c_str());
    if (s) {
      char *p = s;
      while (p) {
        char *p1 = strchr(p, ';');
        char *pv;
        if (p1) {
          *p1 = 0;
          p1++;
        }
        if (haveValue && (NULL != (pv = strchr(p, '?')))) {
          String valueExpansion = "";
          while (pv) {
            *pv = 0;
            valueExpansion = valueExpansion + String(p) + value;
            p = pv + 1;
            pv = strchr(p, '$');
          }
          valueExpansion = valueExpansion + String(p);
          analyzeCmd(valueExpansion.c_str());
        } else
          analyzeCmd(p);
        mp3loop();                // in case of "stop" wait for sdcard to stop playing
        p = p1;  
      }      
      free(s);
    }    
}
  
/*
bool executeCmdsFromNVSKey(const char *key, String value) {
  bool ret = false;
  if (nvssearch(key)) 
     if (ret = (nvsgetstr(key).length() > 0))
      executeCmds(nvsgetstr(key), value);
  return ret;
}
*/

bool executeCmdsFromEvent(const char *event, bool executeOnlyExtra = false, String value = "") {
  bool ret = executeOnlyExtra;
/*
  char s[20];
  strcpy(s+2, event);
  s[0] = '@';
  s[1] = hmiLock?'2':'0' + (localfile?1:0);
  if (!ret)
    ret = executeCmdsFromNVSKey(s, value);
  if (!hmiLock) {
    s[0] = '+';
    executeCmdsFromNVSKey(s, value);
  }
  if (!ret && !hmiLock) {
    s[1] = '@';
    ret = executeCmdsFromNVSKey(s + 1, value); 
  }
  if (!hmiLock) {
    s[1] = '+';
    executeCmdsFromNVSKey(s + 1, value);
  }
*/
  return ret;
}

void setSwitchPositions(String value) {
  appendPairList(switchReadings, value);
}

void setTunePositions(String idxStr, String value) {
  int idx = atoi(idxStr.c_str());
  if ((idx < 0) || (idx > 9))
    idx = 0;
  appendPairList(tuneReadings[idx], value);
}



enum clalibFlags {
  CALIBRATE_SWITCH = 1,
  CALIBRATE_VOLUME = 2,
  CALIBRATE_TUNING = 4
};


void doPrint(char *s) {
  if (DEBUG)
    dbgprint(s);
  else
    Serial.println(s);
}

uint8_t calibrationFlags = 0;
void setCalibration(String value) {
  char s[500];
  value.toLowerCase();  
  calibrationFlags = 0;
  if ((value == "all") || (value == "1"))
    calibrationFlags = CALIBRATE_SWITCH | CALIBRATE_VOLUME | CALIBRATE_TUNING;
  else {
    if (value.indexOf('s') >= 0) 
      calibrationFlags |= CALIBRATE_SWITCH;
    if (value.indexOf('v') >= 0)
      calibrationFlags |= CALIBRATE_VOLUME;
    if (value.indexOf('t') >= 0)
      calibrationFlags |= CALIBRATE_TUNING;
  }
  sprintf(s, "Calibration for Switch is o%s, Pin is: %d", (calibrationFlags & CALIBRATE_SWITCH?"n":"ff"), ini_block.retr_switch_pin);
  doPrint(s);  
  sprintf(s, "Calibration for Tuning is o%s, Pin is: %d", (calibrationFlags & CALIBRATE_TUNING?"n":"ff"), ini_block.retr_tune_pin);
  doPrint(s);  
  sprintf(s, "Calibration for Volume is o%s, Pin is: %d", (calibrationFlags & CALIBRATE_VOLUME?"n":"ff"), ini_block.retr_vol_pin);
  doPrint(s);
}
 

struct medionExtraPins {
  touch_pad_t capaPin;
  touch_pad_t tonePin;
} medionPins = 
  {

    .capaPin = TOUCH_PAD_NUM6, //T4, GPIO14
    .tonePin = TOUCH_PAD_NUM5, //T5, GPIO12
  };

bool retroRadioSetupDone = false;


std::vector<char *> equalizers;
static uint16_t currentEqualizer = 0;

void addEqualizer(String value) {
  char *p = strdup(value.c_str());
  if (p) {
    dbgprint("Add equalizer[%d]: %s", equalizers.size(), p);
    equalizers.push_back(p);
  }
}

void setEqualizer(String setMode, int idx) {
  Serial.printf("setEqualizer(%s, %d). CurrentEqualizer is: %d, equalizers definde: %d\r\n", setMode.c_str(), idx, currentEqualizer, equalizers.size());
  if (equalizers.size() > 0)
  {
    uint16_t lastEqualizer = currentEqualizer;
    if (setMode == "up") 
    {
      if (currentEqualizer + 1 < equalizers.size())
        currentEqualizer++;
    }
    else if (setMode == "upwrap") 
    {
      if (currentEqualizer + 1 < equalizers.size())
        currentEqualizer++;
      else
        currentEqualizer = 0;
    }
    else if (setMode == "down") 
    {
      if (currentEqualizer > 0)
        currentEqualizer--;
    }
    else if (setMode == "downwrap") 
    {
      if (currentEqualizer )
        currentEqualizer--;
      else
        currentEqualizer = equalizers.size() - 1;
    }
    else if (0 == idx) 
    {
      if (currentEqualizer + 1 < equalizers.size())
        currentEqualizer++;
      else
        currentEqualizer = 0;      
    }
    else if (idx <= equalizers.size())
      currentEqualizer = idx - 1;
    if (currentEqualizer != lastEqualizer) 
      analyzeCmds(equalizers[currentEqualizer]);
  }
}



uint16_t tpCapaRead() {
  uint16_t x;
  if (retroRadioSetupDone && (medionPins.capaPin != TOUCH_PAD_MAX))
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
    .waveSwitchPositions = 0,
    .mp3Waveband = MP3WAVEBAND
  };   

#define MEDION_WAVE_SWITCH_POSITIONS  (medionConfig.waveSwitchPositions)

uint16_t medionSwitchPositionCalibrationDefault[] = {
    4090, 4095,       //WaveBand 0 ganz links
    1750, 2050,        //WaveBand 1 in der Mitte 
        0, 5,         //WaveBand 2 ganz rechts 
};


/*
void doInput(String value) {
  Serial.printf("INPUT with value %s\r\n", value.c_str());
  dummy->setParameters(value);
}

void doRead(String value) {
  Serial.printf("Read Input Value=%d\r\n", dummy->read());

}
*/

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
//#define MEDIONCALIBRATE_WAVEBAND
//#define MEDIONCALIBRATE_CAPACITOR
//#define MEDIONCALIBRATE_TONE

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

void doRandomPlay() {
  if (localfile) 
    analyzeCmd("mp3track", "0");
  else {
    if (randMode.length() > 1) {
      //Serial.printf("Executing randmode: %s\r\n", randMode.c_str());
      //executeCmds(randMode);
    }
    else {
      //Serial.println("Selecting random preset!");
      selectRandomPreset();
    }
  }
}

void doToggleHost(bool isDebug) {
  if (localfile) 
    selectPreset(0xff, isDebug);
  else if (SD_nodecount > 0) {
    analyzeCmd("mp3track", "0");
  }
}
/*
void doChannel(String value, int ivalue) {
  if (localfile || (useTuneReadings >= 10) || (lastSetChannel == -1))
    return;
  value.toLowerCase();
  if ((useTuneReadings < 10) && (lastSetChannel != -1)) {
    std::vector<int16_t> readings = getTuneReadings(useTuneReadings);
    int lastChannel = readings.size() / 2 - 1;
    int16_t channel = -1;
    if (value == "up") {
      channel = lastSetChannel + 1;
      if (channel > lastChannel)
        channel = 0;
    } else if (value == "down") {
      channel = lastSetChannel - 1;
      if (channel < 0)
        channel = lastChannel;
    } else if (value == "this") { 
      channel = lastSetChannel;
    } else if (value == "rnd") {
      if (lastChannel > 0) {
        channel = lastSetChannel;
        while (channel == lastSetChannel)
          channel = random(lastChannel + 1);
      }
    } else if (value == "toggle") {
      if (lastSetChannel != 0)
        channel = 0;
      else 
        channel = lastChannel;
    }
    if (channel >= 0) {
      handleApiEvent(API_EVENT_CHANNEL, channel, true); 
    }
  }
}
*/
void doRandMode(String value) {
  chomp(value);
  randMode = value;
}

void doLockHmi(String value, int ivalue) {
  if (value == "on")
    hmiLock = true;
  else if (value == "off")
    hmiLock = false;
  else if (value == "toggle")
    hmiLock = !hmiLock;
  else
    hmiLock = (bool)ivalue;
}


void doLockVol(String value, int ivalue) {
  if (value == "on")
    hmiVolumeLock = true;
  else if (value == "off")
    hmiVolumeLock = false;
  else if (value == "toggle")
    hmiVolumeLock = !hmiVolumeLock ;
  else
    hmiVolumeLock = (bool)ivalue;
}


void showLed(uint8_t idx, uint8_t mode) {
  if (idx < numLeds) {
    static uint32_t lastShowTime[10];
    static std::vector<int16_t>* showPattern[10]; 
    static int16_t sequenceRepeatCount[10];
    static int16_t stepRepeatCount[10];
    static int16_t currentStep[10];
    static bool flashMode[10];
    
    if ((NULL == showPattern[idx]) && (0 == mode))
      return;
    if ((mode != 0) || (millis() - lastShowTime[idx] > 50) && (sequenceRepeatCount[idx] >= 0)) {
      if (mode) {                       // New sequence needs to be started
        showPattern[idx] = &ledPattern[idx];
        sequenceRepeatCount[idx] = (*showPattern[idx])[0];
        stepRepeatCount[idx] = (*showPattern[idx])[1];
        currentStep[idx] = 3;
//        Serial.printf("ledcWrite(%d, %d), patternSize = %d\r\n", idx, (*showPattern[idx])[2], showPattern[idx]->size());
        ledcWrite(idx, (*showPattern[idx])[2]);
      } if (stepRepeatCount[idx] > 0) {
        if (--stepRepeatCount[idx] == 0) {
          currentStep[idx] = currentStep[idx] + 2;
          if (currentStep[idx] > showPattern[idx]->size()) {
            currentStep[idx] = 3;
            if ((*showPattern[idx])[0] > 0)
              if (0 == --sequenceRepeatCount[idx])
                --sequenceRepeatCount[idx];
          }
          stepRepeatCount[idx] = (*showPattern[idx])[currentStep[idx] - 2];
          if (sequenceRepeatCount[idx] >= 0) {
            ledcWrite(idx, (*showPattern[idx])[currentStep[idx] - 1]);
//            Serial.printf("ledcWrite(%d, %d), step = %d\r\n", idx, (*showPattern[idx])[currentStep[idx] - 1], 
//                (currentStep[idx] - 3) / 2);

          }
          else {
            showPattern[idx] = NULL;
            handleApiEvent(API_EVENT_LED, idx, true);
          }
        }
      }
      lastShowTime[idx] = millis();
    }
  }
}

void doLed(uint8_t idx, String value) {
  if (idx < numLeds) {
    ledPattern[idx].clear();
    readDataList(ledPattern[idx], value);
    if (ledPattern[idx].size() == 0)
      ledPattern[idx].push_back(0);
    if (ledPattern[idx].size() == 1) {
      ledPattern[idx].insert(ledPattern[idx].begin(), 0);
    }
    if (ledPattern[idx].size() % 2 == 0) 
      ledPattern[idx].insert(ledPattern[idx].begin(), 1);
    showLed(idx, 1);
  }
}

void handleApiEventTone(int event, uint32_t param, char *s, bool isDebug = false) {
uint16_t pressValue = param & 0xffff;
uint16_t repeatCount = param >> 16;
static bool direct = false;
enum state_tone_t {
  STATE_TONE_IDLE = 0,
  STATE_TONE_DO_MUTE,
  STATE_TONE_DONE
};

static state_tone_t stateTone = STATE_TONE_IDLE;
char key[20];
  bool ret = false;
  if (API_EVENT_TONE_TOUCH == event) {
    char s[10];
    direct = pressValue > TOUCHREAD_DIRECTDELTA;
  }
  bool eventConsumed;
  
  if ((API_EVENT_TONE_LONGPRESSED == event) || (API_EVENT_TONE_2LONGPRESSED == event) || (API_EVENT_TONE_3LONGPRESSED == event)) {
    sprintf(key, "t%c.%d.%d", (direct?'0':'1'), event, repeatCount);
    Serial.printf("  TOUCH EVENT: %s\r\n", key);
    //eventConsumed = executeCmdsFromEvent(key, false);
    sprintf(key, "t%c.%d", (direct?'0':'1'), event, repeatCount);
    Serial.printf("  TOUCH EVENT: %s\r\n", key);
    //eventConsumed = executeCmdsFromEvent(key, eventConsumed);
  } else {
    sprintf(key, "t%c.%d", (direct?'0':'1'), event);
    Serial.printf("  TOUCH EVENT: %s\r\n", key);
    //eventConsumed = executeCmdsFromEvent(key, false);
  }
  //executeCmdsFromEvent("touch", eventConsumed);
  strcpy(s, "");
  
  return;



strcpy(s, "");
  switch (stateTone) {
    case STATE_TONE_IDLE:
      direct = pressValue > TOUCHREAD_DIRECTDELTA;
      if (API_EVENT_TONE_TOUCH == event) {
        if (hmiLock) 
          toggleMedionLED(LED_MODE_DARK);
        else
          if (muteflag) {
            muteflag = false;
            stateTone = STATE_TONE_DONE;  
            strcpy(s, "Undo mute!");
          }
      } else if (hmiLock) {
        if (API_EVENT_TONE_UNTOUCH == event) {
          toggleMedionLED(LED_MODE_FORCELOW);
          strcpy(s, "API is locked!");
        }
        else if ((API_EVENT_TONE_LONGPRESSED == event) && direct && (5 == repeatCount)) {
          hmiLock = false;
          sprintf(s, "HMI unlocked!");
          toggleMedionLED(LED_MODE_FORCELOW);
          toggleMedionLED(LED_MODE_FLASH2);
          stateTone = STATE_TONE_DONE;   
        }  
      }
      else if (API_EVENT_TONE_UNTOUCH != event) {
          if (direct) {
            if (API_EVENT_TONE_2PRESSED == event) {
              analyzeCmd("togglehost", isDebug?"1":"0");
/*
              if (localfile) 
                selectPreset(0xff, isDebug);
              else if (SD_nodecount > 0) {
                analyzeCmd("mp3track", "0");
              }
*/
            }
            if ((API_EVENT_TONE_3PRESSED == event)) {
              analyzeCmd("mute");  
              //stateTone = STATE_TONE_DONE;
              strcpy(s, "Starting mute!");
            } else if (API_EVENT_TONE_PRESSED == event) {
              analyzeCmd("random");
/*              
              if (localfile) {
                analyzeCmd("mp3track", "0");
                //stateTone = STATE_TONE_DONE;
                strcpy(s, "Random play something else from SD!");
              } else {
                selectRandomPreset();
                //stateTone = STATE_TONE_DONE;
                strcpy(s, "Random play one of the known presets!");                
              }
*/              
            } else if (API_EVENT_TONE_LONGPRESSED == event) {
              analyzeCmd("downvolume", "2");
//              if ((ini_block.reqvol < MINVOLUME) && (ini_block.reqvol > 0))
//                analyzeCmd("volume", "0");
            } else if (API_EVENT_TONE_2LONGPRESSED == event) {
              analyzeCmd("upvolume", "2");
//              if (ini_block.reqvol < MINVOLUME) {
//                char s[30];
//                sprintf(s, "%d", MINVOLUME);
//                analyzeCmd("volume", s);
//              }
            }
          } else {
            if (API_EVENT_TONE_LONGPRESSED == event) {
              Serial.printf("LongPressed on Gehäuse, param=%ld\r\n", repeatCount);
              
              if (repeatCount == 0)
                selectPreset(waveBand * NUMCHANNELS, isDebug);
            }
            if (API_EVENT_TONE_PRESSED == event) {
//              if ( equalizers.size() > 0 ) 
//                setEqualizer((currentEqualizer + 1 ) % equalizers.size());
              Serial.println("Pressed on Gehäuse");
            }
            else if (API_EVENT_TONE_2PRESSED == event)
              Serial.println("2Pressed on Gehäuse");
            else if (API_EVENT_TONE_3PRESSED == event) {
              hmiLock = !hmiLock;
//              toggleMedionLED(LED_MODE_FORCELOW);
              toggleMedionLED(LED_MODE_FLASH2);
              Serial.printf("3Pressed on Gehäuse, hmiLock=%d\r\n", hmiLock);
            }
            else if (API_EVENT_TONE_3LONGPRESSED == event) {
              if (repeatCount == 0) {
//                toggleMedionLED(LED_MODE_FORCELOW);
                toggleMedionLED(LED_MODE_FLASH1);
                hmiVolumeLock = !hmiVolumeLock;
                Serial.printf("3longPressed on Gehäuse, apiVolumeLock=%d\r\n", hmiVolumeLock);
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




void handleApiEvent(int event, uint32_t param, bool ignoreLock) {
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
    if (hmiLock && !ignoreLock)
      strcpy(s, "API is locked!");
    else {
      if (useTuneReadings < 10) {
        char key[20];
        int channel = param & 0xff;
        lastSetChannel = channel;
        toggleMedionLED(LED_MODE_HIGH);
        sprintf(key, "tune%d.%d", useTuneReadings, channel);
        bool haveCommand = false;//executeCmdsFromEvent(key, false);
        sprintf(key, "tune%d", useTuneReadings);
        haveCommand = false;//executeCmdsFromEvent(key, haveCommand);
        haveCommand = false;//executeCmdsFromEvent("tune", haveCommand);
        if (!haveCommand)
          if (useTuneReadings < 10) {
          for (int i = 0;i < useTuneReadings;i++)
            channel = channel + getTuneReadings(i).size() / 2;
          char s[10];
          sprintf(s, "%d", channel);
          analyzeCmd("preset", s);
        }
      }
    }
  } else if (event == API_EVENT_SWITCH) {
    sprintf(s, "API event Switch: %ld", param); 
    if (hmiLock && !ignoreLock)
      strcpy(s, "API is locked!");
    else {
        char key[20];
        sprintf(key, "switch%d", param);
        //executeCmdsFromEvent(key);
      }
  } else if (event == API_EVENT_VOLUME) {
    char s1[40];
    sprintf(s1, "volume=%ld", param);
    sprintf(s, "API event volume: %ld", param);
    if (hmiLock && !ignoreLock) {
      ;//strcpy(s, "API is locked!");
    } else if (hmiVolumeLock)
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
      handleApiEventTone(event, param, s);//, s, debuginfo);
  else if (API_EVENT_LED == event) {
    char key[10];
//    sprintf(s, "API event for LED%d: sequence done.", param & 0xff);        
    
    sprintf(key, "led%d", param & 0xff);
    //executeCmdsFromEvent(key);
  }
  else if (API_EVENT_START == event) {
    char key[10];
    sprintf(s, "API event START.");        
    //executeCmdsFromEvent("start");
  }
  else 
    sprintf(s, "Unknown API event: %d with param: %ld\r\n", event, param);
  if ((strstr(s, "API") == s) && (NULL != strstr(s, "is locked!")) && !isTouchEvent) 
    toggleMedionLED(LED_MODE_FLASH3);
  if (debuginfo)
    if (strlen(s))
      doPrint(s);
  if (switchChannel) {
    selectPreset(tunedChannel + waveBand * NUMCHANNELS, debuginfo);
  }
}

void setupMedionExtension() {
//nvs_handle nvsHandler;
  if (retroRadioSetupDone)
    return;

  //analyzeCmd("debug", "0");
/*  
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
  }*/
  touch_pad_init();
  touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
  touch_pad_filter_start(16);  
  adc_power_on();
/*
  for(tunePin = TOUCH_PAD_NUM0;tunePin < TOUCH_PAD_MAX;tunePin = (touch_pad_t)((int)tunePin + 1))
    if (touchpin[(int)tunePin].gpio == ini_block.retr_tune_pin)
      break;
  if (tunePin != TOUCH_PAD_MAX) {
      touch_pad_config(tunePin, 0);
    } 
*/
  if (ini_block.retr_led0_pin != -1) {
    numLeds = 1;
    ledcAttachPin(ini_block.retr_led0_pin, 0); // Attach LED to Driver channel 0
    Serial.println("LEDO gefunden!");
    ledcSetup(0, 4000, 8);
  }
/*  
  if (medionPins.tonePin != TOUCH_PAD_MAX)
    touch_pad_config(medionPins.tonePin, 0);
  if (-1 != ini_block.retr_vol_pin)
    pinMode(ini_block.retr_vol_pin, INPUT);
  if (-1 != ini_block.retr_switch_pin)
    pinMode(ini_block.retr_switch_pin, INPUT);
    //capaReadPin = new VirtualTouchPin(medionPins.capaPin);
*/
//   readPresetList();  
  retroRadioSetupDone = true;
  Serial.println("About to start ::start");
  analyzeCmds(nvsgetstr("::start"));
//  executeCmdsFromEvent("start");
//  tuneKnobRead();
//  dummy = new RetroRadioInputTouch("t6");
//  dummyVol = new RetroRadioInputADC("a33");
//  dummySwitch = new RetroRadioInputADC("a35");
//  dummySwitch->setParameters("@$defswitch");
//  dummySwitch->setParameters("show.1");
}


void handleMedionLoop() {
static bool first = true;
static bool statusWasNeverPublished = true;
static int ledToShow = 0;
    if (first)
      setupMedionExtension();
    if (first || (1 == playingstat) || (localfile))
      {
//        toggleMedionLED(LED_MODE_LOW);
        first = false;
      }
//   volumeRead();
//   if ((1 == playingstat) && (0 != lastVolume))   
//   readChannel(); 
/*
   toneRead();    
   switchKnobRead();
   volumeKnobRead();
   tuneKnobRead();
   if (numLeds) {
    showLed(ledToShow, 0);
    if (++ledToShow >= numLeds)
      ledToShow = 0;
   }
   */
   //dummy->check();
   //dummyVol->check();
   //dummySwitch->check();
   static uint32_t last39;
   if (millis() - last39 > 1000) 
   {
//      Serial.printf("GPIO39: %d\r\n", analogRead(39));
      //Serial.printf("GPIO36: %d\r\n", analogRead(36));
      //Serial.printf("GPIO35: %d\r\n", analogRead(35));
      //Serial.printf("GPIO34: %d\r\n", analogRead(34));
      last39 = millis();
   }
  RetroRadioInput::checkAll();
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
    if (hmiLock) {
      ret = mp3playapiLockLowTable;
      tableSize = sizeof(mp3playapiLockLowTable) / sizeof(mp3playapiLockLowTable[0]);    
    } else if (hmiVolumeLock) {
      ret = mp3playvolLockLowTable;
      tableSize = sizeof(mp3playvolLockLowTable) / sizeof(mp3playvolLockLowTable[0]);    
    } else {
      ret = mp3playdefaultLowTable;
      tableSize = sizeof(mp3playdefaultLowTable) / sizeof(mp3playdefaultLowTable[0]);
    }
  } else {
    if (hmiLock) {
      ret = apiLockLowTable;
      tableSize = sizeof(apiLockLowTable) / sizeof(apiLockLowTable[0]);    
    } else if (hmiVolumeLock) {
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

  return;

}


void switchKnobRead() {
char s[200];
static bool init = true;
  if (-1 != ini_block.retr_switch_pin) {
    static uint16_t readCount = 0;
    static uint32_t readTotal = 0;
    uint16_t switchReadValue = analogRead(ini_block.retr_switch_pin); 
    readTotal = readTotal + switchReadValue;
    readCount++;
    if (init || (readCount >= (localfile?10:100))) {
        uint8_t idx;
        readTotal = readTotal / readCount;
        for(idx = switchReadings.size() / 2 - 1;idx != 0xff;idx = idx - 1)
          if ((switchReadings[idx * 2] <= readTotal) && (switchReadings[idx * 2 + 1] >= readTotal ))
            break;
        if ((calibrationFlags & CALIBRATE_SWITCH )!= 0){
          static uint32_t lastShowTime = 0;
          if (init || (millis() - lastShowTime > 1000)) {
            sprintf(s, "SwitchKnob phys. read=%d, resultingPosition=%d (lastPosition=%d)", (int)readTotal, idx, switchPosition);
            doPrint(s);
            lastShowTime = millis();               
          }
        }
      if ((idx != 0xff) && (idx != switchPosition)) {
        if (calibrationFlags & CALIBRATE_SWITCH != 0){
          sprintf(s, "New Switch position detected! from %d->%d", switchPosition, idx);
          doPrint(s);
        } 
//        if (!init)
          handleApiEvent(API_EVENT_SWITCH, idx); 
        switchPosition = idx;
      }
      readTotal = 0;
      readCount = 0;
    }
  }
  init = false;
}

void volumeKnobRead() {
char s[200];
static bool init = true;
static uint8_t minVolume;
  if (-1 != ini_block.retr_vol_pin) {
    if (init) {
      if (0 < (minVolume = ini_block.vol_min))
        if (ini_block.vol_zero)
          minVolume--;
    }
    static uint16_t readCount = 0;
    static uint32_t readTotal = 0;
    uint16_t volReadValue = analogRead(ini_block.retr_vol_pin); 
    readTotal = readTotal + volReadValue;
    readCount++;
    if (init || (readCount >= (localfile?5:100))) {
        int8_t idx;      
        readTotal = readTotal / readCount;
        idx = map(readTotal, 0, 4095, minVolume, ini_block.vol_max);
        if ((calibrationFlags & CALIBRATE_VOLUME )!= 0){
          static uint32_t lastShowTime = 0;
          if (init || (millis() - lastShowTime > 1000)) {
            sprintf(s, "VolumeKnob phys. read=%d, resultingVolume=%d (lastVolume=%d)", (int)readTotal, idx, volumePosition);
            doPrint(s);
            lastShowTime = millis();               
          }
        }
      if ((abs(idx - volumePosition) > 1) || ((volumePosition != idx) && (idx == minVolume))){
        static uint32_t lastChangeTime = 0;
        if ((abs(idx - volumePosition) > 2) || (millis() - lastChangeTime > 100)) {
          if ((calibrationFlags & CALIBRATE_VOLUME) != 0){
            sprintf(s, "New Volume position detected! from %d->%d", volumePosition, idx);
            doPrint(s);
          }  
          handleApiEvent(API_EVENT_VOLUME, idx);         
          volumePosition = idx;
          lastChangeTime = millis();
        }
      }
      readTotal = 0;
      readCount = 0;
    }
  }
  init = false;
}



void evaluateTuning() {
  tunedChannel = nearestChannel = -1;
  if (physTuneReading != -1)
    if (useTuneReadings < 10) {
      std::vector<int16_t>& readings = getTuneReadings(useTuneReadings);
      if (readings.size() != 0) {
        int16_t lowestDelta = 0x7fff;
        int idx = readings.size() / 2 - 1;
        for(;idx >= 0;idx--) {
          int16_t myDelta;
          if (readings[idx * 2] > physTuneReading)                // reading below lower limit?
            myDelta = readings[idx * 2] - physTuneReading;
          else if (readings[idx * 2 + 1] < physTuneReading)       // reading above upper limit?
            myDelta = physTuneReading - readings[idx * 2 + 1];
          else                                                                          // hit: idx points to valid read channel
            break;
          if (myDelta < lowestDelta) {
            lowestDelta = myDelta;
            nearestChannel = idx;
          }
        }
        tunedChannel = idx;
        if (tunedChannel >= 0)
          nearestChannel = -1;
        int16_t channel = tunedChannel;
        if ((channel == -1) && (lastTunedChannel == -1))
          channel = nearestChannel;
        if ((channel != -1) && (channel != lastTunedChannel)) {
          uint32_t param = ((uint32_t)lastTunedChannel * 0x100) & 0xff00;
          handleApiEvent(API_EVENT_CHANNEL, channel + param);
          lastTunedChannel = channel;
        }
      }
    }
}


void tuneKnobRead() {
char s[200];
static bool init = true;
  if (!init && (TOUCH_PAD_MAX == tunePin))
    return;
  if (-1 == ini_block.retr_tune_pin) {
    init = false;
    return;
  }
  // here init was done with valid pin.
  if (tunePin != TOUCH_PAD_MAX) {
    static uint16_t readCount = 0;
    static uint32_t readTotal = 0;
    uint16_t tuneReadValue;
    touch_pad_read_filtered(tunePin, &tuneReadValue);
    readTotal = readTotal + tuneReadValue;
    readCount++;
    if (/*init ||*/ (readCount >= (localfile?10:100))) {
        readTotal = readTotal / readCount;
        physTuneReading = readTotal;
        evaluateTuning();
        if ((calibrationFlags & CALIBRATE_TUNING )!= 0){
          static uint32_t lastShowTime = 0;
          if (init || (millis() - lastShowTime > 1000)) {
            sprintf(s, "TuneKnob phys. read=%d (channel: %d, nearest channel: %d)", (int)readTotal, tunedChannel, nearestChannel);
            doPrint(s);
            lastShowTime = millis();               
            
          }
        }

      readTotal = 0;
      readCount = 0;
    }
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
      handleApiEvent(API_EVENT_CHANNEL, channelparam);         
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

String retroradioSetup(String argument, String value, int iValue) {
  String ret = String("OK.");
  if (retroRadioSetupDone)
    return String("This command can only be executed from preferences: ") + argument;
  argument = argument.substring(3);  
  if ( argument.startsWith ( "eq" ) ) {
    addEqualizer(value);
  } else if ( argument == "led_inv") {
    ini_block.retr_led_inv = (bool)iValue;
  } else if ( argument.startsWith("sw_pos")) {
    setSwitchPositions(value); 
  } else if ( argument.startsWith("tunepos") )
    setTunePositions(argument.substring(7, 8), value);
  return ret;
}
#endif
