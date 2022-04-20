#include "BLEDevice.h"
#include "BLEScan.h"
#include <UbiConstants.h>
#include <UbiTypes.h>
#include <UbidotsEsp32Mqtt.h>
#include <WiFi.h>

const char *UBIDOTS_TOKEN = "BBFF-uPnUOyq1G4EwIXVOnQqEmmQxxKyH67";
const char *WIFI_SSID = "iPhone";
const char *WIFI_PASS = "sasa8888";
const char *DEVICE_LABEL = "ESP32";
const char *ubiaccpoint = "AccelerationX";
const char *ubiproxpoint = "Proximity";

const int PUBLISH_FREQUENCY = 5000;
unsigned long previousMillis = 0;

float AccelerationX = 0;
int Proximity = 0;

boolean regen_acc = false;
boolean regen_prox = false;

static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
static BLEUUID CharProximityUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");
static BLEUUID CharAccelerationUUID("2ea70455-850e-498b-86c0-bd0f89d76b86");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* aRemoteCharacteristic;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;
Ubidots ubidots(UBIDOTS_TOKEN);

//void callback(char *topic, byte *payload, unsigned int length)
//{
//  Serial.print("Message arrived [");
//  Serial.print(topic);
//  Serial.print("] ");
//  for (int i = 0; i < length; i++)
//  {
//    Serial.print((char)payload[i]);
//  }
//  Serial.println();
//}

static void notifyCallbackp( BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pDatap, size_t length, bool isNotify) {
    Proximity = *pDatap;
    regen_prox = true;
}

static void notifyCallbacka( BLERemoteCharacteristic* aBLERemoteCharacteristic, uint8_t* pDataa, size_t length, bool isNotify) {
    AccelerationX  = *(float*)pDataa;
    regen_acc = true;
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    pClient->connect(myDevice); 
    Serial.println(" - Connected to server");

    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    Serial.print("passed");
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");

    aRemoteCharacteristic = pRemoteService->getCharacteristic(CharAccelerationUUID);
    if (aRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(CharAccelerationUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our acceleration characteristic");

    if(aRemoteCharacteristic->canRead()) {
      std::string value = aRemoteCharacteristic->readValue();
      Serial.print("The acceleration characteristic value was: ");
      Serial.println(value.c_str());
    }

    pRemoteCharacteristic = pRemoteService->getCharacteristic(CharProximityUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(CharProximityUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our proximity characteristic");

    // Read the value of the characteristic.
    if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      Serial.print("The proximity characteristic value was: ");
      Serial.println(value.c_str());
    }

    if(aRemoteCharacteristic->canNotify())
      aRemoteCharacteristic->registerForNotify(notifyCallbacka);
    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallbackp);
    
    connected = true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {

  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    }
  }
}; 


void setup() {
  Serial.begin(115200);
  while(!Serial);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");
  ubidots.setDebug(true);
  ubidots.connectToWifi(WIFI_SSID, WIFI_PASS);
//  ubidots.setCallback(callback);
  ubidots.setup();
  ubidots.reconnect();

  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(300, false);
} 


void loop() {
  
  if (doConnect == true) {
    if (connectToServer()) { 
      Serial.println("We are now connected to the BLE Server.");
    } 
    else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
      doConnect = false;
  }
  if (!ubidots.connected()){
    ubidots.reconnect();
    }

  if (regen_acc && regen_prox) {
    Serial.print("Acceleration:");
    Serial.println(AccelerationX);
    Serial.print("Proximity:");
    Serial.println(Proximity);
    regen_acc = false;
    regen_prox = false;
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= PUBLISH_FREQUENCY) {
    ubidots.add(ubiaccpoint, AccelerationX);
    ubidots.add(ubiproxpoint, Proximity);
    previousMillis = currentMillis;
    ubidots.publish(DEVICE_LABEL);
  }
ubidots.loop();

}
