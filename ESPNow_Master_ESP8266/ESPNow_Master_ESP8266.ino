/*
   * Author: Andreas Spiess, 2017
   * 
  This sketch measures the time to send a ESP-Now message. It is a strip down of Anthony's sensor sketch

https://github.com/HarringayMakerSpace/ESP-Now
Anthony Elder
*/
#include <ESP8266WiFi.h>
extern "C" {
#include <espnow.h>
}
//#include "SparkFunBME280.h"

// this is the MAC Address of the remote ESP server which receives these sensor readings
uint8_t remoteMac[] = {0x1A, 0xFE, 0x34, 0xD5, 0xFA, 0x39};

#define WIFI_CHANNEL 1
#define SLEEP_SECS 5  // 15 minutes
#define SEND_TIMEOUT 245  // 245 millis seconds timeout 

#define MESSAGELEN 10

unsigned long entry;

// keep in sync with slave struct
struct __attribute__((packed)) SENSOR_DATA {
  char testdata[MESSAGELEN];
} sensorData;

//BME280 bme280;

volatile boolean callbackCalled;

unsigned long entry1 = millis();

void setup() {
  int i = 0;
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.println("ESP_Now Controller");
    Serial.println();
  WiFi.mode(WIFI_STA); // Station mode for esp-now sensor node
  WiFi.disconnect();

  Serial.printf("This mac: %s, ", WiFi.macAddress().c_str());
  Serial.printf("target mac: %02x%02x%02x%02x%02x%02x", remoteMac[0], remoteMac[1], remoteMac[2], remoteMac[3], remoteMac[4], remoteMac[5]);
  Serial.printf(", channel: %i\n", WIFI_CHANNEL);

  if (esp_now_init() != 0) {
    Serial.println("*** ESP_Now init failed");
    gotoSleep();
  }
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  Serial.println(millis() - entry1);
  unsigned long entry2 = millis();
  esp_now_add_peer(remoteMac, ESP_NOW_ROLE_SLAVE, WIFI_CHANNEL, NULL, 0);
  Serial.println(millis() - entry2);
  unsigned long entry3 = millis();

  esp_now_register_send_cb([](uint8_t* mac, uint8_t sendStatus) {
    Serial.printf("send_cb, send done, status = %i\n", sendStatus);
    callbackCalled = true;
  });
  Serial.println(millis() - entry3);
  unsigned long entry4 = millis();

  callbackCalled = false;

  for (i = 0; i < MESSAGELEN; i++) sensorData.testdata[i] = '0';
  sensorData.testdata[MESSAGELEN] = '\0';
}

void loop() {

  uint8_t bs[sizeof(sensorData)];
  memcpy(bs, &sensorData, sizeof(sensorData));
  unsigned long entry = millis();
  esp_now_send(NULL, bs, sizeof(sensorData)); // NULL means send to all peers
  Serial.print("Time to send: ");
  Serial.println(millis() - entry);
  Serial.print("Overall Time: ");
  Serial.println(millis() - entry1);
  //  Serial.print("Size: ");
  //  Serial.println(sizeof(bs));
  ESP.deepSleep(0);
}
/*
  void loop() {
  if (callbackCalled || (millis() > SEND_TIMEOUT)) {
    gotoSleep();
  }
  }
*/
void gotoSleep() {
  // add some randomness to avoid collisions with multiple devices
  int sleepSecs = SLEEP_SECS;// + ((uint8_t)RANDOM_REG32/2);
  Serial.printf("Up for %i ms, going to sleep for %i secs...\n", millis(), sleepSecs);
  ESP.deepSleep(sleepSecs * 1000000, RF_NO_CAL);
}
