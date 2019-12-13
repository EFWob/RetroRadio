#include <nvs.h>
#define NVS_ERASE     true            // NVS will be completely deleted if set to true (only use for development, must be false in field SW)
static uint8_t waveBand = 0xff;

struct medionExtraPins {
  uint8_t led;
  uint8_t vol;
  uint8_t wave;
  uint8_t capa;
  uint8_t loudUp;
  uint8_t loudDn;
} medionPins = 
  {
    .led = 26,
    .vol = 33,
    .wave = 35,
    .capa = T4,
    .loudUp = 0, //T5,
    .loudDn = 0  //T6
  };

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
    3150, 3550,
    2550, 3000,
    0, 50
};

uint16_t *medionSwitchPositionCalibration = (uint16_t *)&medionSwitchPositionCalibrationDefault;




uint8_t medionChannelLimitsDefault[] = { 
//    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    6 ,7 , 
    9, 9,
    11, 12, 
    15, 16, 
    20, 22, 
    26, 28, 
    32, 34, 
    37, 55};

uint8_t * medionChannelLimits = (uint8_t *)&medionChannelLimitsDefault;        

//#define MEDIONCALIBRATE_VOLUME
//#define MEDIONCALIBRATE_WAVEBAND
//#define MEDIONCALIBRATE_CAPACITOR
//#define MEDIONCALIBRATE_TONE


bool medionExtensionSetupDone = false;

void setupMedionExtension() {
nvs_handle nvsHandler;
  if (medionExtensionSetupDone)
    return;
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
      len = medionConfig.numChannels * sizeof(uint8_t) * 2;
      if (medionChannelLimits = (uint8_t *)malloc(len))
        nvs_get_blob(nvsHandler, "capa", medionChannelLimits, &len);
      } 
    else
      {
      nvs_set_blob(nvsHandler, "config", &medionConfig, sizeof(medionConfig));  
      len = medionConfig.waveSwitchPositions * sizeof(uint16_t) * 2;
      nvs_set_blob(nvsHandler, "switch", &medionSwitchPositionCalibrationDefault, len);
      len = medionConfig.numChannels * sizeof(uint8_t) * 2;
      nvs_set_blob(nvsHandler, "capa", &medionChannelLimitsDefault, len);

      }
    nvs_commit(nvsHandler);  
  }
}

static uint8_t lastVolume = 0xff;

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
  if (medionPins.vol)
    {
    currentRead = analogRead(medionPins.vol);
    if (currentRead < 4000)
      {
        count++;
        total = total + currentRead;    
      }
    }  
  if (medionPins.wave) {
    totalWave += analogRead(medionPins.wave);   
  }
    if (++ticks == 75) {
      if (count > 0) 
        {
#ifdef MEDIONCALIBRATE_VOLUME
          Serial.print("VolumeReadPhys: ");Serial.print(total / count);
#endif
          total = (total / count) / 87;
          if (total > 0)
            total += 55;
          count = 0;
          if ((abs(lastVolume-total) > 1) /*|| (total == 0)*/) 
            {
              char s[30];
              sprintf(s, "volume=%d", total);
#ifdef MEDIONCALIBRATE_VOLUME
              Serial.print("Setting: ");Serial.println(s);
#endif
              lastVolume = total;
              analyzeCmd(s);
            }
          }
     if (medionPins.wave) 
      {
        uint32_t waveRead = totalWave / ticks;
//        uint16_t waveRead = analogRead(medionPins.wave);
#if defined(MEDIONCALIBRATE_WAVEBAND)
        Serial.print("WaveReadPhys: ");     Serial.println(waveRead);
#endif
        for(waveBandNew = MEDION_WAVE_SWITCH_POSITIONS-1;waveBandNew < 0xff;waveBandNew--)
          if ((waveRead >= medionSwitchPositionCalibration[waveBandNew * 2]) &&
               (waveRead <= medionSwitchPositionCalibration[waveBandNew * 2 + 1]))
               break;
        if ((waveBandNew != lastWaveBand) && (waveBandNew < MEDION_WAVE_SWITCH_POSITIONS)) 
          {
          waveBand = lastWaveBand = waveBandNew;  
#if defined(MEDIONCALIBRATE_WAVEBAND)
          Serial.print("NEW WaveBand: ");Serial.println(waveBand);
#endif
          }
     }
     totalWave = 0;
     total = 0;
     ticks = 0;        
    }
}
  
void readChannel() {
  if (medionPins.capa)
    {
    static uint8_t lastReportedChannel = 0xff;
    static uint8_t lastDebounceReadChannel = 0xff;
    static uint8_t lastReportedWaveband = 0;
    uint16_t capRead = touchRead(medionPins.capa);
    static uint32_t totalCapRead = 0;
    static uint32_t lastCapRead = 0;
    static uint16_t ticks = 0;
    uint8_t channel;
    bool hit = false;

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
    if (lastCapRead == 0)
      lastCapRead = capRead;
    else
      if (lastCapRead == capRead) {
#if defined(MEDIONCALIBRATE_CAPACITOR)
        Serial.println("No Change in CapRead!");
#endif
        //return;        
      } else if (lastCapRead > capRead)
        lastCapRead--;
      else
        lastCapRead++;
    capRead = lastCapRead;
#if defined(MEDIONCALIBRATE_CAPACITOR)
    Serial.print("CapRead: ");Serial.println(capRead);
#endif
    channel = 0;
    while (!hit && (channel < NUMCHANNELS) && (capRead >= medionChannelLimits[channel*2])) {
      hit = (capRead <= medionChannelLimits[channel * 2 + 1]);
      if (!hit)
        channel++;
    }
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

