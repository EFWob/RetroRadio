#include <nvs.h>
#define NVS_ERASE     true            // NVS will be completely deleted if set to true (only use for development, must be false in field SW)
#define NUMCHANNELS   15
#define NUMWAVEBANDS   3
#define MP3WAVEBAND    1


//#define TOUCHLEVEL_TONE       450     // pressed detected if below this level
#define TOUCHLEVEL_TONEDIRECT 400     // primary area, if pressed and below that level

#define TOUCHTIME_TONELONG1   400
#define TOUCHTIME_TONELONGFF  250

static uint8_t waveBand = 0; //0xff;
static uint8_t tunedChannel = 0;

struct medionExtraPins {
  uint8_t led;
  uint8_t vol;
  uint8_t wave;
  touch_pad_t capaPin;
  touch_pad_t tonePin;
  touch_pad_t loudUp;
  touch_pad_t loudDn;
} medionPins = 
  {
    .led = 26,
    .vol = 33,
    .wave = 35,
//    .capa = 0xff,       // obsolete
    .capaPin = TOUCH_PAD_NUM4, //T4, GPIO13
    .tonePin = TOUCH_PAD_NUM5, //T5, GPIO12
    .loudUp = TOUCH_PAD_MAX, //T5,
    .loudDn = TOUCH_PAD_MAX  //T6
  };

bool medionExtensionSetupDone = false;


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
    3500, 4100,
    1600, 2000,
    0, 50
};


enum api_event_t {
  API_EVENT_CHANNEL = 0,
  API_EVENT_WAVEBAND,
  API_EVENT_VOLUME,
  API_EVENT_TONE_TOUCH,
  API_EVENT_TONE_UNTOUCH,
  API_EVENT_TONE_PRESSED,
  API_EVENT_TONE_LONGPRESSED,
  API_EVENT_TONE_LONGRELEASED
};


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
#define MEDIONCALIBRATE_TONE



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
      direct = pressValue > 90;
      if (API_EVENT_TONE_TOUCH == event) {
        if (muteflag) {
          muteflag = false;
          stateTone = STATE_TONE_DONE;  
          strcpy(s, "Undo mute!");
        }
      } else if (API_EVENT_TONE_UNTOUCH != event) {
          if (direct) {
            if ((API_EVENT_TONE_LONGPRESSED == event) && (0 == repeatCount)) {
              analyzeCmd("mute");  
              stateTone = STATE_TONE_DONE;
              strcpy(s, "Starting mute!");
            }
          }
        }
      break;
    case STATE_TONE_DONE:
      if (API_EVENT_TONE_UNTOUCH == event)
        stateTone = STATE_TONE_IDLE;
      break;
  }
#ifdef OOOOLD  
  char *eventName = "";
  switch (event) {
    case API_EVENT_TONE_TOUCH:
      eventName = "Tone touched";
      direct = pressValue > 80;
      break;
    case API_EVENT_TONE_UNTOUCH:
      eventName = "Tone Untouched";
      direct = false;
      break;
    case API_EVENT_TONE_PRESSED:
      if (direct)
        analyzeCmd("mute");
      eventName = "Tone (short) pressed";
      break;
    case API_EVENT_TONE_LONGPRESSED:
      eventName = "Tone long pressed";
      break;
    case API_EVENT_TONE_LONGRELEASED:
      eventName = "Tone long released";
      break;
  };
  if (isDebug)
    sprintf(s, "API event tone (direct: %d): %s (pressValue: %d, repeatCount: %d)", direct, eventName, pressValue, repeatCount);
#endif
}


void handleApiEvent(api_event_t event, uint32_t param, bool init = false) {
char s[500];  
bool debuginfo;
bool switchChannel = false;
static bool playMp3 = false;
#if defined(MEDION_DEBUGAPI)
  debuginfo = true;  
#else
  debuginfo = false;  
#endif
  if (event == API_EVENT_CHANNEL) {
    sprintf(s, "API event channel: %04X (from %d to %d)", param, (param & 0xff00) / 0x100, param & 0xff);        
    tunedChannel = param & 0xff;
    if (playMp3) {
      switchChannel = false;
      analyzeCmd("mp3track", "0");
    } else
      switchChannel = !init;
  } else if (event == API_EVENT_WAVEBAND) {
    sprintf(s, "API event waveband: %ld", param); 
    if (SD_okay && (0 < SD_nodecount) && ((param & 0xff) == medionConfig.mp3Waveband)) {
      switchChannel = false;
      analyzeCmd("mp3track", "0");  
      playMp3 = true;
    } else {
      switchChannel = !init;
      playMp3 = false;
    }
  } else if (event == API_EVENT_VOLUME) {
    char s1[40];
    sprintf(s1, "volume=%ld", param);
    sprintf(s, "API event volume: %ld", param);
    if (!muteflag) 
      analyzeCmd(s1);
    else {
      strcat(s, " (ignored since muteflag is set!)");
      muteflag = (param != 0);
    }
  } else if ((API_EVENT_TONE_TOUCH == event) ||
             (API_EVENT_TONE_UNTOUCH == event) ||
             (API_EVENT_TONE_PRESSED == event) ||
             (API_EVENT_TONE_LONGPRESSED == event) ||
             (API_EVENT_TONE_LONGRELEASED == event)
             
             ) 
      handleApiEventTone(event, param, s, debuginfo);//, s, debuginfo);
  else
    sprintf(s, "Unknown API event: %d with param: %ld\r\n", event, param);
  if (debuginfo)
    dbgprint(s);
  if (switchChannel) {
    uint16_t channel = tunedChannel + waveBand * NUMCHANNELS;
    if (debuginfo)
      Serial.printf("API switch to channel: %d\r\n", channel);
    sprintf(s, "preset=%d", channel);
    analyzeCmd(s);
  }
}

void setupMedionExtension() {
nvs_handle nvsHandler;
  if (medionExtensionSetupDone)
    return;
  
  
  medionExtensionSetupDone = true;

  touch_pad_init();
  touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
  touch_pad_filter_start(10);  

  
  if (medionPins.capaPin != TOUCH_PAD_MAX)
    touch_pad_config(medionPins.capaPin, 0);
  if (medionPins.tonePin != TOUCH_PAD_MAX)
    touch_pad_config(medionPins.tonePin, 0);

    //capaReadPin = new VirtualTouchPin(medionPins.capaPin);
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
}

static uint8_t lastVolume = 0;

void handleMedionLoop() {
static bool first = true;
static bool statusWasNeverPublished = true;
    if (first)
      setupMedionExtension();
    if (first || (1 == playingstat))
      {
        toggleMedionLED(LOW);
        first = false;
      }
   volumeRead();
//   if ((1 == playingstat) && (0 != lastVolume))   
      readChannel(); 
   toneRead();    
}

void toggleMedionLED(int8_t x) {
  if (!medionPins.led)
    return;
  if (0xff == x)  
    digitalWrite(medionPins.led, !digitalRead(medionPins.led));
  else
    digitalWrite(medionPins.led, x);
/*
  if (LOW == x)
    digitalWrite(medionPins.led, HIGH);
  else if (HIGH == x)
    digitalWrite(medionPins.led, LOW);
  else
    digitalWrite(medionPins.led, !digitalRead(medionPins.led));
*/
  pinMode(medionPins.led, OUTPUT);
}

void volumeRead() {

static uint8_t lastWaveBand = 0xff;
uint8_t waveBandNew;  
static uint32_t lastRead = 0;
static uint16_t ticks = 0;
static uint16_t count = 0;
static uint32_t total = 0;
static uint32_t totalWave = 0;
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
    totalWave += analogRead(medionPins.wave);   
  }
    if ((++ticks == 75) || init) {
      if (count > 0) 
        {
#ifdef MEDIONCALIBRATE_VOLUME
        static uint32_t lastReportTime = 0;
        if (millis() > lastReportTime + 2000) {
          Serial.printf("VolumeReadPhys: %ld\r\n", total / count);
          lastReportTime = millis();
        }
#endif
          total = (total / count) / 82;
          if (total > 0)
            total += 51;
          count = 0;
          if ((abs(lastVolume-total) > 1) /*|| (total == 0)*/) 
            {

#ifdef MEDIONCALIBRATE_VOLUME
              Serial.print("Setting: ");Serial.println(s);
#endif
              lastVolume = total;
              handleApiEvent(API_EVENT_VOLUME, total, init);         
            }
          }
     if (medionPins.wave != 0xff) 
      {
        uint32_t waveRead = totalWave / ticks;
//        uint16_t waveRead = analogRead(medionPins.wave);
        for(waveBandNew = MEDION_WAVE_SWITCH_POSITIONS-1;waveBandNew < 0xff;waveBandNew--)
          if ((waveRead >= medionSwitchPositionCalibration[waveBandNew * 2]) &&
               (waveRead <= medionSwitchPositionCalibration[waveBandNew * 2 + 1]))
               break;

#if defined(MEDIONCALIBRATE_WAVEBAND)
        static uint32_t lastReportTime = 0;
        if (millis() > lastReportTime + 2000) {
          Serial.printf("WaveReadPhys: %d, thisIsSwitchPosition: %d\r\n", waveRead, waveBandNew);
          lastReportTime = millis();
        }
#endif               
        if ((waveBandNew != lastWaveBand) && (waveBandNew < MEDION_WAVE_SWITCH_POSITIONS)) 
          {
          waveBand = lastWaveBand = waveBandNew;  
          handleApiEvent(API_EVENT_WAVEBAND, waveBand, init); 
#if defined(MEDIONCALIBRATE_WAVEBAND)
          Serial.print("NEW WaveBand: ");Serial.println(waveBand);
#endif
          }
     }
     totalWave = 0;
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
      TONEREAD_STATE_LONGIDLE
      
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
        if (toneRead < touchLevelUnpressed - 30) {  
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
        if (toneRead > touchLevelUnpressed - 30) {
          toneReadState = TONEREAD_STATE_IDLE;
        } else if (millis() - stateStartTime > 50) {
          stateStartTime = millis();
          toneReadState = TONEREAD_STATE_PRESSED;
          handleApiEvent(API_EVENT_TONE_TOUCH, touchLevelUnpressed - toneReadMin);
        }
      break;
    case TONEREAD_STATE_PRESSED:
        if (toneRead > touchLevelUnpressed - 30) {
          handleApiEvent(API_EVENT_TONE_PRESSED, touchLevelUnpressed - toneReadMin);
          handleApiEvent(API_EVENT_TONE_UNTOUCH, touchLevelUnpressed - toneReadMin);
          toneReadState = TONEREAD_STATE_IDLE;
        } else if (millis() - stateStartTime > TOUCHTIME_TONELONG1) {
          toneReadState = TONEREAD_STATE_LONGIDLE;
          handleApiEvent(API_EVENT_TONE_LONGPRESSED, touchLevelUnpressed - toneReadMin);
          longRepeats = 0;
          stateStartTime = millis();
        }
      break;  
    case TONEREAD_STATE_LONGIDLE:
        if (toneRead > touchLevelUnpressed - 30) {
          handleApiEvent(API_EVENT_TONE_LONGRELEASED, longRepeats + touchLevelUnpressed - toneReadMin);
          handleApiEvent(API_EVENT_TONE_UNTOUCH, touchLevelUnpressed - toneReadMin);
          toneReadState = TONEREAD_STATE_IDLE;
        } else if (millis() - stateStartTime > TOUCHTIME_TONELONGFF) {
          longRepeats = longRepeats + 0x10000;
          handleApiEvent(API_EVENT_TONE_LONGPRESSED, longRepeats + touchLevelUnpressed - toneReadMin);
          stateStartTime = millis();          
        }
      break;  
    default:
      break;
  }

}
