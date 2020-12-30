#include <nvs.h>
#define NVS_ERASE     true            // NVS will be completely deleted if set to true (only use for development, must be false in field SW)
static uint8_t waveBand = 0; //0xff;
static uint8_t tunedChannel = 0;

struct medionExtraPins {
  uint8_t led;
  uint8_t vol;
  uint8_t wave;
  uint8_t capa;           //obsolete!
  touch_pad_t capaPin;
  uint8_t loudUp;
  uint8_t loudDn;
} medionPins = 
  {
    .led = 26,
    .vol = 33,
    .wave = 35,
    .capa = 0xff,       // obsolete
    .capaPin = TOUCH_PAD_NUM4, //T4,
    .loudUp = TOUCH_PAD_MAX, //T5,
    .loudDn = TOUCH_PAD_MAX  //T6
  };

//#include "VirtualPin.h"
//VirtualTouchPin *capaReadPin = NULL;
bool medionExtensionSetupDone = false;


uint16_t tpCapaRead() {
  uint16_t x;
  if (medionExtensionSetupDone && (medionPins.capaPin != TOUCH_PAD_MAX))
    touch_pad_read_filtered(medionPins.capaPin, &x);
  else
    x = 0xffff;
  return x;
//  return (capaReadPin?capaReadPin->analogRead():0xffff);
}

uint16_t tp4read() {
  return tpCapaRead();
}

struct medionConfigType {
    uint8_t numChannels;
    uint8_t touchLevel;
    uint32_t touchDebounceTime;
    int8_t touchDebounce;
    uint8_t waveSwitchPositions;      
}  medionConfig = 
  {
    .numChannels = 8,
    .touchLevel = 25,
    .touchDebounceTime = 50,
    .touchDebounce = 2,
    .waveSwitchPositions = 3
  };   

#define MEDION_LOUDNESS_TOUCH_LEVEL (medionConfig.touchLevel)
#define MEDION_WAVE_SWITCH_POSITIONS  (medionConfig.waveSwitchPositions)
#define NUMCHANNELS (medionConfig.numChannels)

uint16_t medionSwitchPositionCalibrationDefault[] = {
    3500, 4100,
    1600, 2000,
    0, 50
};


enum api_event_t {
  API_EVENT_CHANNEL = 0,
  API_EVENT_WAVEBAND,
  API_EVENT_VOLUME,
};


uint16_t *medionSwitchPositionCalibration = (uint16_t *)&medionSwitchPositionCalibrationDefault;




uint16_t medionChannelLimitsDefaultOld[] = { 
//    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    6 ,7 , 
    9, 9,
    11, 12, 
    15, 16, 
    20, 22, 
    26, 28, 
    32, 34, 
    37, 55};

uint16_t * medionChannelLimitsOld = (uint16_t *)&medionChannelLimitsDefaultOld;        

uint16_t medionChannelLimitsDefaultNew[] = { 
//    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    88 ,98 , 
    105, 117,
    128, 144, 
    158, 181, 
    204, 234, 
    252, 273, 
    294, 310, 
    326, 342};

uint16_t * medionChannelLimitsNew = (uint16_t *)&medionChannelLimitsDefaultNew;        

#define MEDION_DEBUGAPI
//#define MEDIONCALIBRATE_VOLUME
//#define MEDIONCALIBRATE_WAVEBAND
//#define MEDIONCALIBRATE_CAPACITOR
//#define MEDIONCALIBRATE_TONE



//#define API_EVENT_CHANNEL  0
//#define API_EVENT_WAVEBAND 1
//#define API_EVENT_VOLUME   2

void handleApiEvent(api_event_t event, uint32_t param, bool init = false) {
char s[500];  
bool debuginfo;
bool switchChannel = false;
#if defined(MEDION_DEBUGAPI)
  debuginfo = true;  
#else
  debuginfo = false;  
#endif
  if (event == API_EVENT_CHANNEL) {
    sprintf(s, "API event channel: %ld", param);        
    tunedChannel = param;
    switchChannel = !init;
  } else if (event == API_EVENT_WAVEBAND) {
    sprintf(s, "API event waveband: %ld", param);    
    switchChannel = !init;
  } else if (event == API_EVENT_VOLUME) {
    sprintf(s, "volume=%ld", param);
    analyzeCmd(s);
    sprintf(s, "API event volume: %ld", param);
  } else
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
      nvs_set_blob(nvsHandler, "capa", &medionChannelLimitsDefaultNew, len);

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
    static uint8_t lastDebounceReadChannel = 0xff;
    static uint8_t lastReportedWaveband = 0;
    static uint32_t totalCapRead = 0;
    static uint32_t lastCapRead = 0;
    static uint16_t ticks = 0;
    uint8_t channel;
    bool hit = false;
    bool timeToReport = false;

  uint16_t * medionChannelLimits = NULL;
  uint16_t capRead;

  if (TOUCH_PAD_MAX != medionPins.capaPin) //NULL != capaReadPin)
  {
    medionChannelLimits = medionChannelLimitsNew;
    capRead = tpCapaRead();
//    capRead = capaReadPin->analogRead();
//    static uint32_t lastRead = millis();
//    if ((millis() - lastRead) > 2000) {
//      lastRead = millis();
//      Serial.printf("TouchRead=%d\r\n", capRead);
//    }
  }
  
  else if (medionPins.capa != 0xff)
    {
    medionChannelLimits = medionChannelLimitsOld;
    capRead = touchRead(medionPins.capa);
    }
  if (NULL == medionChannelLimits)      // no capaRead for frequency tuning?
    return;
#if !defined(MEDIONCALIBRATE_CAPACITOR)
  if ((capRead < medionChannelLimits[0]) || (capRead > medionChannelLimits[NUMCHANNELS * 2 - 1]))
    return;
#endif
  ticks++;  
  totalCapRead += capRead;
  if (ticks < 15)
    return;
  capRead = totalCapRead / ticks;
  totalCapRead = 0;
  ticks = 0;
#if defined(MEDIONCALIBRATE_CAPACITOR)
 
#endif

//      else if (lastCapRead > capRead)
//        lastCapRead--;
//      else
//        lastCapRead++;
//    capRead = lastCapRead;
    channel = 0;
    while (!hit && (channel < NUMCHANNELS) && (capRead >= medionChannelLimits[channel*2])) {
      hit = (capRead <= medionChannelLimits[channel * 2 + 1]);
      if (!hit)
        channel++;
    }
#if defined(MEDIONCALIBRATE_CAPACITOR)
 static uint32_t lastReportTime = 0;
  if (millis() > lastReportTime + 2000) {
    Serial.print("CapRead: ");Serial.println(capRead);
    if (lastCapRead == 0)
      lastCapRead = capRead;
    else
      if (lastCapRead == capRead) {
//        Serial.println("No Change in CapRead!");
        //return;        
        };
    lastReportTime = millis();      
    timeToReport = true;
    Serial.printf("CapRead= %d (changed: %d), isHit(chan[%d (of %d)]) = %d\r\n", capRead, (capRead != lastCapRead), channel, NUMCHANNELS, hit);
  }
  lastCapRead = capRead;
#endif
    if (hit && (lastReportedChannel != channel)) {
      handleApiEvent(API_EVENT_CHANNEL, channel, lastReportedChannel == 0xff);         
      lastReportedChannel = channel;
    }
    return;
    if (((hit) /* && (1 == playingstat)*/ && (lastReportedChannel != channel)) ||     // either a change in channel
        ((lastReportedWaveband != waveBand) && (lastReportedChannel != 0xff))){       // or in waveband-setting
          char s[20];
          if (!hit)                                                                   // change in waveband setting only?
            channel = lastReportedChannel;                                            // just to be sure we have the right channel
          else {
            if ((lastDebounceReadChannel != channel) && (lastDebounceReadChannel != 0xff)) {
              lastDebounceReadChannel = channel;
              return;
            }
          }
          lastReportedChannel = channel;
          lastDebounceReadChannel = channel;
          lastReportedWaveband = waveBand;  
          channel = channel + NUMCHANNELS * waveBand;
#if defined(MEDIONCALIBRATE_CAPACITOR)
          Serial.print("Channel (log): ");Serial.print(channel);Serial.print(" Channel (phys): ");Serial.println(channel - NUMCHANNELS * waveBand);
#endif
          toggleMedionLED(HIGH);
          sprintf(s, "preset=%d", channel);     
          analyzeCmd(s);    
      }
}


void toneRead() {
  if (medionPins.loudUp && medionPins.loudDn)
    {
    static int8_t x = 0;                          //Debounce-Counter. 
    static uint32_t lastRead = 0;                 //time of last check
    bool setChange = false;                       //User request to change Loudness-Setting qualified?
  
    if (lastRead == 0) {                        //Very first call. (aka setup()) Use tonela to set initial loudness
      lastRead = 1;
      x = ini_block.rtone[2];
      setChange = true;
    }

    if (millis() - lastRead > medionConfig.touchDebounceTime) {  //Cyclic check of touchpads needed?  
      uint8_t touch;
      static bool readUp = true;                //Each round only read either of them
      lastRead = millis();
      if (readUp)                               //check if "LoudnessUp" is touched
        {
           touch = touchRead(medionPins.loudUp);
//          Serial.print("TouchUp: ");Serial.println(touch);
          if (touch < MEDION_LOUDNESS_TOUCH_LEVEL)
            if (x >= 0)                         //either same has been touched last round or it is the first touch detected
              x++;                              //increment debounce counter
            else
              x= 0;                             //reset debounce counter (this will prevent any reaction if both are pressed at the same time)
          readUp = false;                       //next round check for loudness down
        }
      else                                      //same for down
        {
          touch = touchRead(medionPins.loudDn);
//          Serial.print("TouchDn: ");Serial.println(touch);
          if (touch < MEDION_LOUDNESS_TOUCH_LEVEL)
            if (x <= 0)
              x--;
            else
              x = 0;
          readUp = true;
        }
#if defined(MEDIONCALIBRATE_TONE)
      Serial.println(String("Touch: ") + touch + String(", x:") + x + ", Lvl: " + MEDION_LOUDNESS_TOUCH_LEVEL);
#endif
      if ((x > medionConfig.touchDebounce) || (x < -medionConfig.touchDebounce))                  //qualified after some debounce?
        {
          x = (x > 0)?1:-1;                     //is it up or down?
          x += ini_block.rtone[2];
          if ((x >= 0) && (x < 16))             //not wrapped around?
            {
              char s[20];
              setChange = true;
              sprintf(s, "tonela=%d", x);       //set new bass amplitude
#if defined(MEDIONCALIBRATE_TONE)
              Serial.print("Loudness: ");Serial.println(x);
#endif

              analyzeCmd(s);
            }
          else 
            x = 0;
        }
    }
    if (setChange)                              //change in treble amplitude required?
      {
        char s[20];
        sprintf(s, "toneha=%d", (x - 8) & 15);    
        analyzeCmd(s);                          //set new treble
//        if (millis() > 600000)
//          savetime = millis() - 595000 ;                          // Limit save to once per 10 minutes
        x = 0;
      }
    }
}
