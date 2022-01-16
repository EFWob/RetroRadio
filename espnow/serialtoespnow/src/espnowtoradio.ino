#if defined(ESP32)
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#else
#include <ESP8266WiFi.h>
extern "C" {
  #include <espnow.h>
}
#endif



#define WIFI_CHANNEL 9
#define ESPNOW_REPEATS 4
#define ESPNOW_REPEATTIME 40
uint8_t broadcastMac[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

void setup() {
    Serial.begin(115200);delay(500);Serial.println("\r\n");
    WiFi.mode(WIFI_STA);
    WiFi.begin("hello", "world", WIFI_CHANNEL, NULL, false);
#if defined(ESP32)
    esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
    if (ESP_OK != esp_now_init())
      Serial.println("ESP-Now-Init fail!!!!");
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, broadcastMac, 6);
    peerInfo.channel = WIFI_CHANNEL;  
    peerInfo.encrypt = false;
    //esp_wifi_set_max_tx_power(0);
    if (esp_now_add_peer(&peerInfo) != ESP_OK)
      Serial.println("Error adding ESP-Now peer!");
#else
    //WiFi.setOutputPower(0);
    esp_now_init();
    esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
    esp_now_add_peer(broadcastMac, ESP_NOW_ROLE_COMBO, WIFI_CHANNEL, NULL, 0);

#endif

  Serial.printf("WiFi MAC is: %s\r\n", WiFi.macAddress().c_str());
  Serial.printf("WiFi Channel is: %d\r\n", WiFi.channel());
}


void sendToRadio(String prefix, String message) {
static char c=0;
int len = prefix.length() + message.length() +2;
  if (len < 249)
  {
    unsigned char buf[255];
    uint8_t plen = prefix.length() + 1;
    buf[0] = plen;
    memcpy(buf + 1, prefix.c_str(), plen);
    buf[plen] = c;
    memcpy(buf + plen + 1, message.c_str(), message.length());
    for (int i = 0;i < ESPNOW_REPEATS;i++) {
      esp_now_send(broadcastMac,buf, len);
      if (i < ESPNOW_REPEATS-1)
        delay(ESPNOW_REPEATTIME);  
    }    
    Serial.printf("To radio (len: %d): <%d>%s<%d>%s\r\n", len, plen, prefix.c_str(), c, message.c_str());
    c++;
  }
}

String serialReadline = "";
void loop() {
  // put your main code here, to run repeatedly:
  if (Serial.available()) {
    char c = Serial.read();
    if ((c == '\n')) {
      serialReadline.trim();
      if (serialReadline.length() > 0) {
        if (serialReadline.c_str()[0] == '-') {
          String prefix = serialReadline.substring(1);
          prefix.trim();
          int idx = prefix.indexOf(' ');
          if (idx > 0) {
            String message = prefix.substring(idx + 1);
            prefix = prefix.substring(0, idx);
            prefix.trim();
            message.trim();
            if ((prefix.length() > 0) && (message.length() > 0))
              sendToRadio(prefix, message);
          }
        }
        else
          sendToRadio("RADIO", serialReadline);
      serialReadline = "";
      }
    }
    else if (c >= ' ')
      serialReadline = serialReadline + c;
  }
}
