/*******************************************************************
 * 
 * Author: Andreas Spiess, 2017
 * 
 * This sketch is used to measure the time needed to send a message to IFTTT
 * 
  inspired by: https://github.com/pinae/ESP8266-Dash
 *******************************************************************/




#include <IFTTTMaker.h>  // https://github.com/witnessmenow/arduino-ifttt-maker 




#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <credentials.h>

//------- Replace the following! ------
//#define IFTTT_KEY "..............."  // Get it from this page https://ifttt.com/services/maker/settings
#define EVENT_NAME "mail" // Name of your event name, set when you are creating the applet

#define AIO_SERVER      "maker.ifttt.com"

#define LED 2
long entry1 = millis();


WiFiClientSecure client;
IFTTTMaker ifttt(IFTTT_KEY, client);

void setup() {
  Serial.begin(115200);
  Serial.println("IFTTT Button example");
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);

  // Set WiFi to station mode and disconnect from an AP if it was Previously
  // connected
  WiFi.mode(WIFI_STA);
  //  WiFi.disconnect();
  WiFi.begin(mySSID, myPASSWORD);
  Serial.print("Connecting Wifi: ");
  int retries = 50;
  while (WiFi.status() != WL_CONNECTED && --retries > 0) {
    Serial.print(".");
    delay(500);
  }
  if (retries == 0) ESP.restart();
  Serial.println("");
  Serial.println("WiFi connected ");
      Serial.println("Time to connect to the AP");
  Serial.println(millis() - entry1);
  unsigned long entry2 = millis();
  Serial.print("Connecting IFTTT: ");

  retries = 10;
  digitalWrite(LED, HIGH);
  while (!ifttt.triggerEvent(EVENT_NAME, "X", "Y", "Z") && retries > 0) {

    Serial.print("!");
  }
  if (retries > 0)  {
    Serial.println("Successfully sent");
    //   blinkSuccess();
  }
  else {
    Serial.println("Got no success message.");
    blinkError();
  }
  Serial.println("Time to send to IFTTT");
  Serial.println(millis() - entry2);
    Serial.println("Total Transmission time");
  Serial.println(millis() - entry1);
  Serial.println("Going to sleep.");
  ESP.deepSleep(0);
}

void loop() {
  yield();
}

void blinkSuccess() {
  for (int i = 4; i < 50; i = (5 * i) >> 2) {
    digitalWrite(LED, HIGH);   // turn the LED off
    delay(10 * i);             // wait
    digitalWrite(LED, LOW);    // turn the LED on
    delay(10 * i);             // wait
  }
}

void blinkError() {
  for (int i = 0; i < 28; i++) {
    digitalWrite(LED, HIGH);   // turn the LED off
    delay(125);                        // wait
    digitalWrite(LED, LOW);    // turn the LED on
    delay(125);                        // wait
  }
}

void shutdown() {
  Serial.println("Shutting down.");
  Serial.println("Going to sleep.");
  //digitalWrite(PORT, LOW); // make sure, the ESP is enabled even if button is released
  ESP.deepSleep(0);
  Serial.println("Sleep failed.");
  while (1) {
    blinkError();
  }
}



