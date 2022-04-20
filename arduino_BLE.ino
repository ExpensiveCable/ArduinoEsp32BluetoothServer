#include <ArduinoBLE.h>
#include <Arduino_LSM9DS1.h>
#include <Arduino_APDS9960.h>

BLEService ServiceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
BLEByteCharacteristic ProximityCharUuid("beb5483e-36e1-4688-b7f5-ea07361b26a8", BLERead | BLENotify | BLEWrite); 
BLEFloatCharacteristic XAccCharUuid("2ea70455-850e-498b-86c0-bd0f89d76b86", BLERead | BLENotify | BLEWrite);

unsigned long initial_acc = 0;
unsigned long initial_prox = 0;
unsigned long previousMillis = 0;
float x, y, z;
int interval = 1000;

void setup() {
  Serial.begin(9600);
//  while (!Serial);
  
  if (!BLE.begin()) {
    Serial.println("* Starting BLE module failed!");
    while (1);
  } 
  if (!IMU.begin()) {
    Serial.println("The IMU could not be initialized");
    while (1);
  }
  if (!APDS.begin()) {
    Serial.println("The proximity sensor could not be initialized");
    while (1);
  }
  
  BLE.setLocalName("Nano 33 BLE (Central) Carl"); 
  BLE.setAdvertisedService(ServiceUUID);

  ServiceUUID.addCharacteristic(ProximityCharUuid);
  ServiceUUID.addCharacteristic(XAccCharUuid);

  BLE.addService(ServiceUUID);
  ProximityCharUuid.writeValue(initial_prox);
  XAccCharUuid.writeValue(initial_acc);
  
  BLE.advertise();

  Serial.println("Arduino Nano 33 BLE Sense (Central Device)");
}

void getdata(){
  
  if (IMU.accelerationAvailable() & APDS.proximityAvailable()) {
      IMU.readAcceleration(x,y,z);
      XAccCharUuid.writeValue(x);
      Serial.print("Acceleration:");
      Serial.print(x);
      int proximity = APDS.readProximity();
      ProximityCharUuid.writeValue(proximity);
      Serial.println("Proximity:");
      Serial.print(proximity);
  }
}

void loop() {
    BLEDevice central = BLE.central();
    if (central){
      Serial.print("Connected to central");
      Serial.print(central.address());
      digitalWrite(LED_BUILTIN, HIGH); 
      while (central.connected())
      {
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= interval) 
        {
          getdata();
          previousMillis = currentMillis;
        }   
      }
      digitalWrite(LED_BUILTIN, LOW); 
      Serial.print("diconnected from central");
}
}
