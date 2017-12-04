/*
   LoRa Sender which transmits a test message via the Hope RFM95 LoRa Module.
   It expects a feedback signal from the receiver containing a checksum.

   Used to measure transmission times

   Copyright <2017> <Andreas Spiess>

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.

   Based on the Example of the library
*/



// #define ESP8266 Uncomment if you want to use the sketch for the Wemos shield
// #define BMP   Uncomment if you want to use a BMP085 sensor
// #define INTER uncomment if you want to use interrupt 1 insteaad of a timer interrupt

#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_BMP085.h>


#ifndef ESP8266
#include <LowPower.h>

#endif

const long freq = 868.1E6;
const int SF = 7;
const long bw = 125E3;
long zeroAltitude;

int counter = 1, messageLostCounter = 0;
//#define BMP
// #define INTER

#ifdef BMP
Adafruit_BMP085 bmp;
#endif

void setup() {
  Serial.begin(9600);
  while (!Serial);

#ifdef ESP8266
  LoRa.setPins(16, 17, 15); // set CS, reset, IRQ pin
#else
  LoRa.setPins(10, A0, 2); // set CS, reset, IRQ pin
#endif

  Serial.println("LoRa Sender");

  if (!LoRa.begin(freq)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  LoRa.setSpreadingFactor(SF);
  //  LoRa.setSignalBandwidth(bw);

#ifdef BMP
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP085 sensor, check wiring!");
    while (1) {}
  }

  Serial.print("Pressure = ");
  zeroAltitude = bmp.readPressure();
  Serial.println(zeroAltitude);
  Serial.println(" Pa");
#endif

#ifdef INTER
  pinMode(3, INPUT);
  Serial.println("Interrupt driven");
#endif

  Serial.print("Frequency ");
  Serial.print(freq);
  Serial.print(" Bandwidth ");
  Serial.print(bw);
  Serial.print(" SF ");
  Serial.println(SF);
}

void loop() {
  char message[250];
  int Vcc = readVcc() / 10;
  unsigned long entry;
  int i;
  for (int messageLen = 200; messageLen < 230; messageLen = messageLen + 20) {
#ifdef INTER
    attachInterrupt(1, wakeUp, RISING);
#endif

#ifdef BMP
    //delay(100);
    int alti = (int)bmp.readAltitude(zeroAltitude);
    sprintf(message, "{\"Alt\":\"%3d\",\"Count\":\"%03d\",\"Lost\":\"%3d\",xxx}", alti, counter, messageLostCounter);
#else
    sprintf(message, "{\"Vcc\":\"%3d\",\"Count\":\"%03d\",\"Lost\":\"%03d\",xxx}", Vcc, counter, messageLostCounter);
#endif
    
    for (i = 0; i < messageLen; i++) message[i] = '0';
    message[i] = '\0';

    Serial.print("Size: ");
    Serial.println(messageLen);
    entry = millis();
    sendMessage(message);
    int nackCounter = 0;
    while (!receiveAck(message) && nackCounter <= 5) {

      Serial.println(" refused ");
      Serial.print(nackCounter);
      LoRa.sleep();
      delay(1000);
      sendMessage(message);
      nackCounter++;
    }

    if (nackCounter >= 5) {
      Serial.println("");
      Serial.println("--------------- MESSAGE LOST ---------------------");
      messageLostCounter++;
      delay(100);
    } else {
      Serial.println("Acknowledged ");
    }
    counter++;
    Serial.print("Transmission took: ");
    Serial.println(millis() - entry);
    LoRa.sleep();
    //LoRa.idle();

#ifdef ESP8266
    delay(8000);
#else
    Serial.println("Falling asleep");
    delay(100);
#ifdef INTER
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
    detachInterrupt(1);
#else
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
 //   delay(8000);
#endif
#endif
  }
}

long readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA, ADSC));
  result = ADCL;
  result |= ADCH << 8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  return result;
}

bool receiveAck(String message) {
  String ack;
  Serial.print(" Waiting for Ack ");
  bool stat = false;
  unsigned long entry = millis();
  while (stat == false && millis() - entry < 2000) {
    if (LoRa.parsePacket()) {
      ack = "";
      while (LoRa.available()) {
        ack = ack + ((char)LoRa.read());
      }
      int check = 0;
      // Serial.print("///");
      for (int i = 0; i < message.length(); i++) {
        check += message[i];
        //   Serial.print(message[i]);
      }
      /*    Serial.print("/// ");
          Serial.println(check);
          Serial.print(message);*/
      Serial.print(" Ack ");
      Serial.print(ack);
      Serial.print(" Check ");
      Serial.print(check);
      if (ack.toInt() == check) {
        Serial.print(" Checksum OK ");
        stat = true;
      }
    }
  }
  return stat;
}

void sendMessage(String message) {
//  Serial.print(message);
  // send packet
  LoRa.beginPacket();
  LoRa.print(message);
  LoRa.endPacket();
}

void wakeUp()
{
//  delay(100);
  Serial.println("wakeup");
//  delay(100);
}

