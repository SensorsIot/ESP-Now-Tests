/**
 * 
 * Author: Andreas Spiess, 2017
 * 
  This sketch receives ESP-Now message and sends it as an MQTT messge
  It is heavily based on of Anthony's gateway setch sketch

https://github.com/HarringayMakerSpace/ESP-Now
Anthony Elder
 */
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <credentials.h>
extern "C" {
  #include "user_interface.h"
  #include <espnow.h>
}


//-------- Customise the above values --------
#define SENDTOPIC "ESPNow/key"
#define COMMANDTOPIC "ESPNow/command"
#define SERVICETOPIC "ESPNow/service"

/* Set a private Mac Address
 *  http://serverfault.com/questions/40712/what-range-of-mac-addresses-can-i-safely-use-for-my-virtual-machines
 * Note: the point of setting a specific MAC is so you can replace this Gateway ESP8266 device with a new one
 * and the new gateway will still pick up the remote sensors which are still sending to the old MAC 
 */
uint8_t mac[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x33};
void initVariant() {
  //wifi_set_macaddr(SOFTAP_IF, &mac[0]);
}

#ifdef CREDENTIALS
char *ssid      = mySSID;               // Set you WiFi SSID
char *password  = myPASSWORD;               // Set you WiFi password
#else
char *ssid      = "";               // Set you WiFi SSID
char *password  = "";               // Set you WiFi password
#endif

IPAddress server(192, 168, 0, 203);

// the X's get replaced with the remote sensor device mac address
const char deviceTopic[] = "ESPNOW/";

WiFiClient wifiClient;
PubSubClient client(server, 1883, wifiClient);

String deviceMac;

// keep in sync with ESP_NOW sensor struct
struct __attribute__((packed)) SENSOR_DATA {
   char testdata[240];
} sensorData;

volatile boolean haveReading = false;

/* Presently it doesn't seem posible to use both WiFi and ESP-NOW at the
 * same time. This gateway gets around that be starting just ESP-NOW and
 * when a message is received switching on WiFi to sending the MQTT message
 * to Watson, and then restarting the ESP. The restart is necessary as 
 * ESP-NOW doesn't seem to work again even after WiFi is disabled.
 * Hopefully this will get fixed in the ESP SDK at some point.
 */

int heartBeat;


void setup() {
  Serial.begin(115200); 
  Serial.println();
  Serial.println();
  Serial.println("ESP_Now Controller");
    Serial.println();

  WiFi.mode(WIFI_AP);
  Serial.print("This node AP mac: "); Serial.println(WiFi.softAPmacAddress());
  Serial.print("This node STA mac: "); Serial.println(WiFi.macAddress());

  initEspNow();  
  Serial.println("Setup done");
}


void loop() {
  if (millis()-heartBeat > 30000) {
    Serial.println("Waiting for ESP-NOW messages...");
    heartBeat = millis();
  }

  if (haveReading) {
    haveReading = false;
    wifiConnect();
    reconnectMQTT();
    sendToNodeRed();
    client.disconnect();
    delay(200);
    ESP.restart(); // <----- Reboots to re-enable ESP-NOW
  }
}

void sendToNodeRed() {

  Serial.println(sensorData.testdata);
  String payload = "{";
  payload += "\"test\":\"" + String(sensorData.testdata);
  payload += "\"}";
  Serial.println(payload);
  publishMQTT(SENDTOPIC,payload);
}

void initEspNow() {
  if (esp_now_init()!=0) {
    Serial.println("*** ESP_Now init failed");
    ESP.restart();
  }

  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

  esp_now_register_recv_cb([](uint8_t *mac, uint8_t *data, uint8_t len) {

    deviceMac = "";
    deviceMac += String(mac[0], HEX);
    deviceMac += String(mac[1], HEX);
    deviceMac += String(mac[2], HEX);
    deviceMac += String(mac[3], HEX);
    deviceMac += String(mac[4], HEX);
    deviceMac += String(mac[5], HEX);
    
    memcpy(&sensorData, data, sizeof(sensorData));

    Serial.print("Message received from device: "); Serial.print(deviceMac);
    Serial.println(sensorData.testdata);
    Serial.println();
//    Serial.printf(" Temp=%0.1f, Hum=%0.0f%%, pressure=%0.0fmb\n", 
//       sensorData.temp, sensorData.humidity, sensorData.pressure);    

    haveReading = true;
  });
}

void wifiConnect() {
  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print("Connecting to "); Serial.print(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
     delay(250);
     Serial.print(".");
  }  
  Serial.print("\nWiFi connected, IP address: "); Serial.println(WiFi.localIP());
}

void publishMQTT(String topic, String message) {
  Serial.println("Publish");
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.publish(SENDTOPIC, message.c_str());
}

void reconnectMQTT() {
  Serial.println(" Loop until we're reconnected");
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("Mailbox", "admin", "admin")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(SERVICETOPIC, "I am live");
      // ... and resubscribe
      //  client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc = ");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


