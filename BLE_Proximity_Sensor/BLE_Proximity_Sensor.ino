
/*
 * 
 * This detects advertising messages of BLE devices and compares it with stored MAC addresses. 
 * If one matches, it sends an MQTT message to swithc something

   Copyright <2017> <Andreas Spiess>

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
   
   Based on Neil Kolban's example file: https://github.com/nkolban/ESP32_BLE_Arduino
 */

#include "BLEDevice.h"
#include <WiFi.h>
#include <PubSubClient.h>


static BLEAddress *pServerAddress;

#define LED 22

BLEScan* pBLEScan;
BLEClient*  pClient;
bool deviceFound = false;

String knownAddresses[] = { "  :  :  :  :  :  ", "  :  :  :  :  :  "};

const char* ssid = "***";
const char* password = "***";
const char* mqtt_server = "192.168.0.203";  // change for your own MQTT broker address
#define TOPIC "outTopic"  // Change for your own topic
#define PAYLOAD "1"    // change for your own payload

 unsigned long entry;

WiFiClient espClient;
PubSubClient MQTTclient(espClient);

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  Serial.print("Notify callback for characteristic ");
  Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
  Serial.print(" of data length ");
  Serial.println(length);
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    /**
        Called for each advertising BLE server.
    */
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      Serial.print("BLE Advertised Device found: ");
      Serial.println(advertisedDevice.toString().c_str());
      pServerAddress = new BLEAddress(advertisedDevice.getAddress());

      bool known = false;
      for (int i = 0; i < (sizeof(knownAddresses) / sizeof(knownAddresses[0])); i++) {
        if (strcmp(pServerAddress->toString().c_str(), knownAddresses[i].c_str()) == 0) known = true;
      }
      if (known) {
        Serial.print("Device found: ");
        Serial.println(advertisedDevice.getRSSI());
        if (advertisedDevice.getRSSI() > -80) deviceFound = true;
        else deviceFound = false;
        Serial.println(pServerAddress->toString().c_str());
        advertisedDevice.getScan()->stop();
      }
      /******** This releases the memory when we're done. ********/
      delete pServerAddress; 
   }
}; // MyAdvertisedDeviceCallbacks

void sendMessage() {
  //btStop();
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  entry = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - entry >= 15000) esp_restart();
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected, IP address: ");
  Serial.println(WiFi.localIP());
  MQTTclient.setServer(mqtt_server, 1883);
  MQTTclient.setCallback(MQTTcallback);
  Serial.println("Connect to MQTT server...");
  while (!MQTTclient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (MQTTclient.connect("ESP8266Client", "admin", "admin")) {
      Serial.println("connected");
      MQTTclient.publish(TOPIC, PAYLOAD);
    } else {
      Serial.print("failed, rc=");
      Serial.print(MQTTclient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(1000);
    }
  }
  for (int i = 0; i > 10; i++) {
    MQTTclient.loop();
    delay(100);
  }
  MQTTclient.disconnect();
  delay(100);
  WiFi.mode(WIFI_OFF);
  btStart();
}

void MQTTcallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}


void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);

  BLEDevice::init("");

  pClient  = BLEDevice::createClient();
  Serial.println(" - Created client");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
}

void loop() {

  Serial.println();
  Serial.println("BLE Scan restarted.....");
  deviceFound = false;
  BLEScanResults scanResults = pBLEScan->start(30);
  if (deviceFound) {
    Serial.println("on");
    digitalWrite(LED, LOW);

    sendMessage();
    Serial.println("Waiting for 60 seconds");
    delay(60000);

  }
  else {
    Serial.println("off");
    digitalWrite(LED, HIGH);
  }
} // End of loop
