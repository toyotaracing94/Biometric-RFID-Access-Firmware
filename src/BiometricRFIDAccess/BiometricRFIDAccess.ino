/*
### >>> DEVELOPER NOTE <<< ###
This CODE provided for TELEMATICS EXPERTISE in door Authentication feature on BATCH#2 Development

HARDWARE PIC : AHMADHANI FAJAR ADITAMA 
SOFTWARE PIC : DHIMAS DWI CAHYO
MOBILE APP FEATURE PIC : REZKI MUHAMMAD github: @rezkim

This microcontroller is used as BLE Server Controlling another ESP32. 
ESP32 included :
ESP32 LED Control (location in dashboard)
ESP32 AC Control (location middle in front seat)

<<<<>>>>
TO DO :
- membuat flow process dari code ini
- update code untuk pendaftaran fingerprint (menggunakan modul DFROBOT)
- membuat pesan berhasil dalam JSON
- membuat pesan error dalam JSON
- apakah perlu list error untuk debugging?
*/

/// Requirement for Hardware
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <ArduinoJson.h>
// #include <Adafruit_Fingerprint.h> /* using old model fingerprint hardware*/
#include <Adafruit_PN532.h>
#include <DFRobot_ID809_I2C.h>

/// Requirement for Connection
#include <BLEDevice.h>  
#include <BLEServer.h>  
#include <BLEUtils.h>  
#include <BLE2902.h>  
#include <ArduinoJson.h>


/// CONSTANT VARIABLE
#define Solenoid 15        // Pin Solenoid (prototype)
#define SDA_PIN 21         // Pin SDA to PN532
#define SCL_PIN 22         // Pin SCL to PN532
#define RX_PIN 16          // Pin RX to TX Fingerprint
#define TX_PIN 17          // Pin TX to RX Fingerprint

#define CS_PIN 5           // Chip Select pin
#define SCK_PIN 18         // Clock pin
#define MISO_PIN 19        // Master In Slave Out
#define MOSI_PIN 23        // Master Out Slave In

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"  
#define NOTIFICATION_CHARACTERISTIC "01952383-cf1a-705c-8744-2eee6f3f80c8"
#define LED_REMOTE_CHARACTERISTIC "beb5483e-36e1-4688-b7f5-ea07361b26a8"  
#define DOOR_CHARACTERISTIC "ce51316c-d0e1-4ddf-b453-bda6477ee9b9"
#define AC_REMOTE_CHARACTERISTIC "9f54dbb9-e127-4d80-a1ab-413d48023ad2"  
#define bleServerName "Yaris Cross Door Auth"

#define COLLECT_NUMBER 3 

BLEServer* pServer = NULL;                      // BLE Server
BLEService* pService = NULL;                    // BLE Service
BLECharacteristic* pNotification = NULL;        // Notification Characteristic
BLECharacteristic* LEDremoteChar = NULL;        // LED Characteristic
BLECharacteristic* DoorChar = NULL;             // Door Characteristic
BLECharacteristic* ACChar = NULL;               // AC Characteristic
BLEAdvertising* pAdvertising = NULL;            // Device Advertise

HardwareSerial mySerial(2);                     // HardwareSerial 
//Adafruit_Fingerprint finger(&mySerial);         // Address Fingeprint
Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);           // Address PN532
const size_t JSON_CAPACITY = 1024;              // file JSON
boolean ledState = LOW;                         // --
File dataAkses;                                 // --
boolean enrollMode = false;                     // variabel enrolmode
boolean deleteMode = false;                     // variabel deletemode
boolean listMode = false;                       // variabel listmode

int id;                                         // variabel ID
int solenoidCounter = 0;                        // counter Solenoid
bool deviceConnected = false;                   // Device connection status
String registrant_name = "";
DFRobot_ID809_I2C fingerprint;
JsonObject globalJSON;


// Device state to define process
enum State {
  RUNNING,
  ENROLL_RFID,
  ENROLL_FP,
  DELETE_RFID,
  DELETE_FP,
  SYNC_VISITOR
};

// Define the lock type
enum LockType {
  RFID,
  FINGERPRINT
};

State currentState = RUNNING;               // Initial State

// Task Prototypes
void taskRFID(void* parameter);  
void taskFingerprint(void* parameter);

// Function Prototypes
/// Card Sensor (PN532)
void enrollUserPN532(String visitorName);
void listAccessPN532();  
void deleteUserPN532(String visitorName); 
void verifyAccessPN532();  

/// Fingerprint Sensor (AS608)
// void enrollFingerprint();  
// void deleteFingerprint();  
// void listAccessFingerprint(); //Belum dipake
// void actionFingerprint();  

/// Fingerprint Sensor (Capacitive Fingerprint Scanner NEW!!!)
void fingerprintMatching();
void fingerprintRegistration(String visitorName);
void fingerprintDeletion(uint8_t data);

/// Actuator (Solenoid Doorlock)
void toggleSolenoid(int& counter);
void setupSolenoid();  

/// BLE Connection
void setupBLE();  
void handleCommands();
void handleAcCommand(JsonDocument bleJSON);
void handleLedCommand(JsonDocument bleJSON);

/// SD Card
void setupSDCard();  
void addFingerprintToSDCard(uint8_t fingerprintID);
void deleteFingerprintFromSDCard(uint8_t fingerprintID);
bool addRFIDCardToSDCard(String username, String RFID);
bool deleteRFIDCardFromSDCard(String data);

/// Task Controller
void taskRFID(void *parameter);
void taskFingerprint(void *parameter);
void taskDfRobotFingerprint(void *parameter); 

/* 
>>>> Bluetooth Utilities Section<<<<
*/

/// Server Callback
class MyServerCallbacks : public BLEServerCallbacks {  
  void onConnect(BLEServer* pServer) {  
    deviceConnected = true;  
    Serial.println("BLE: connected!");  

  };  
  
  void onDisconnect(BLEServer* pServer) {  
    deviceConnected = false;  
    Serial.println("BLE: disconnected!");  
    BLEDevice::startAdvertising();  
  }  
};  

/// Characteristic Callback
class CharacteristicCallback : public BLECharacteristicCallbacks {    
  void onWrite(BLECharacteristic* pCharacteristic) {    
    String value = pCharacteristic->getValue();    
    Serial.print("Characteristic value: ");    
    Serial.println(value.c_str());  
    
      // Parse the incoming JSON command  
    // StaticJsonDocument<256> doc;
    JsonDocument bleJSON;
    // Parsing JSON dari nilai yang diterima
    DeserializationError error = deserializeJson(bleJSON, value);
    if (!error) {
      // Menyalin isi bleJSON ke globalJSON
      // NOTE : Potentially become a racing condition
      globalJSON.clear(); // Kosongkan globalJSON sebelum menyalin
      globalJSON.set(bleJSON.as<JsonObject>()); // Menyalin isi bleJSON ke globalJSON
    } else {
      Serial.print("Failed to parse JSON: ");
      Serial.println(error.f_str());
    }

    const char* command = bleJSON["command"]; // Ambil command dari JSON
    JsonVariant data = bleJSON["data"]; // Ambil data dari JSON

    // Cek command dan ubah state sesuai dengan command
    if (strcmp(command, "register_rfid") == 0) {
      currentState = ENROLL_RFID; // Pindah ke mode enroll RFID
    } else if (strcmp(command, "register_fp") == 0) {
      currentState = ENROLL_FP; // Pindah ke mode enroll Fingerprint
    } else if (strcmp(command, "delete_rfid") == 0) {
      currentState = DELETE_RFID; // Pindah ke mode delete RFID
    } else if (strcmp(command, "delete_fp") == 0) {
      currentState = DELETE_FP; // Pindah ke mode delete Fingerprint
    } else if (strcmp(command, "sync_visitor") == 0){
      currentState = SYNC_VISITOR;
    } else{
        if (pCharacteristic->getUUID().toString() == LED_REMOTE_CHARACTERISTIC) {  
          handleLedCommand(bleJSON);
        } else if (pCharacteristic->getUUID().toString() == AC_REMOTE_CHARACTERISTIC) {  
          handleAcCommand(bleJSON);
        } else {  
          Serial.println("Unknown JSON format/message");  
        } 
    }
  } 
};

/// Bluetooth Data Handler
// Function to handle LED commands
void handleLedCommand(JsonDocument bleJSON) {  
  Serial.print("Handling LED command: ");  
  String jsonString;  
  serializeJson(bleJSON, jsonString);
  LEDremoteChar->setValue(jsonString);
  LEDremoteChar->notify();  
}  

// Function to handle Door commands  
void handleAcCommand(JsonDocument bleJSON) {  
  Serial.print("Handling AC command: ");  
  // Serial.println(bleJSON);   
  String jsonString;  
  serializeJson(bleJSON, jsonString);
  ACChar->setValue(jsonString);
  ACChar->notify();  
}

void sendMessagetoClient(const String& command, const JsonVariant& data) {    
  if (deviceConnected) {    
    // Create a JSON document  
    StaticJsonDocument<256> jsonDoc; // Adjust size as needed  
    jsonDoc["command"] = command; // Set the command  
    jsonDoc["data"] = data; // Set the data  
  
    // Serialize the JSON document to a string  
    String jsonString;  
    serializeJson(jsonDoc, jsonString);  
  
    // Check the command and send to the appropriate characteristic  
    if (command == "send_to_LED") {  
      LEDremoteChar->setValue(jsonString);    
      LEDremoteChar->notify();    
    } else {  
      DoorChar->setValue(jsonString);  
      DoorChar->notify();  
    }
  
    Serial.print("Sent to CLIENTA: ");    
    Serial.println(jsonString.c_str());    
  } else {    
    Serial.println("Device not connected");    
  } 
}


/// BLE Configuration 
void setupBle() {  
  Serial.println("BLE initializing...");  
  
  BLEDevice::init(bleServerName);  
  
  pServer = BLEDevice::createServer();  
  pServer->setCallbacks(new MyServerCallbacks());  
  
  pService = pServer->createService(SERVICE_UUID);  
  
  LEDremoteChar = pService->createCharacteristic(  
    LED_REMOTE_CHARACTERISTIC,  
    BLECharacteristic::PROPERTY_NOTIFY |  
    BLECharacteristic::PROPERTY_READ |  
    BLECharacteristic::PROPERTY_WRITE);  
  
  LEDremoteChar->addDescriptor(new BLE2902());  
  LEDremoteChar->setCallbacks(new CharacteristicCallback());  
  
  DoorChar = pService->createCharacteristic(  
    DOOR_CHARACTERISTIC,  
    BLECharacteristic::PROPERTY_NOTIFY |  
    BLECharacteristic::PROPERTY_READ |  
    BLECharacteristic::PROPERTY_WRITE);  
  
  DoorChar->addDescriptor(new BLE2902());  
  DoorChar->setCallbacks(new CharacteristicCallback());  

  ACChar = pService->createCharacteristic(  
    AC_REMOTE_CHARACTERISTIC,  
    BLECharacteristic::PROPERTY_NOTIFY |  
    BLECharacteristic::PROPERTY_READ |  
    BLECharacteristic::PROPERTY_WRITE);  
  
  ACChar->addDescriptor(new BLE2902());  
  ACChar->setCallbacks(new CharacteristicCallback());

  NotificationChar = pService->createCharacteristic(  
    NOTIFICATION_CHARACTERISTIC,  
    BLECharacteristic::PROPERTY_NOTIFY |  
    BLECharacteristic::PROPERTY_READ |  
    BLECharacteristic::PROPERTY_WRITE);  
  
  NotificationChar->addDescriptor(new BLE2902());  
  NotificationChar->setCallbacks(new CharacteristicCallback());
  
  pService->start();  
  
  pAdvertising = BLEDevice::getAdvertising();  
  pAdvertising->addServiceUUID(SERVICE_UUID);  
  pAdvertising->setScanResponse(true);  
  pAdvertising->setMinPreferred(0x06);  
  pAdvertising->setMinPreferred(0x12);  
  
  BLEDevice::startAdvertising();  
  
  Serial.println("BLE initialized. Waiting for client to connect...");  
}  


/*
>>>> State Section <<<<
*/

/// TaskRFID Multitasking
void taskRFID(void *parameter) {
  while (1) {
    if (currentState == RUNNING) {
      verifyAccessPN532(); 
    }
    vTaskDelay(100 / portTICK_PERIOD_MS); 
  }
}

/// Task Fingerprint Multitasking
void taskFingerprint(void *parameter) {
  while (1) {
    if (currentState == RUNNING) {
      // actionFingerprint(); 
      fingerprintMatching();
    }
    vTaskDelay(100 / portTICK_PERIOD_MS); 
  }
}

/*
>>>> NFC Section <<<<
*/
/// Too checking who are registered
void listAccessPN532() {
  File file = SD.open("/data.json", FILE_READ);
  StaticJsonDocument<JSON_CAPACITY> doc;
  if (file) {
    deserializeJson(doc, file);
    file.close();

    Serial.println("Daftar akses:");
    for (JsonPair kv : doc.as<JsonObject>()) {
      Serial.print("Nama: ");
      Serial.println(kv.key().c_str());
      Serial.print("UID: ");
      const char* uid = kv.value()["uid"] | "Tidak tersedia";
      Serial.println(uid);
      Serial.print("ID Fingerprint: ");
      const char* fingerprint = kv.value()["fingerprint_id"].isNull() ? "Tidak tersedia" : kv.value()["fingerprint_id"].as<String>().c_str();
      Serial.println(fingerprint);
      Serial.println();
    }
  } else {
    Serial.println("Gagal membaca data akses.");
  }
  return;
}

/// register UID NFC
void enrollUserPN532(String visitorName) {  
    String name = visitorName;
  
    Serial.println("Dekatkan kartu RFID untuk mendaftar...");  
    uint8_t uid[7];  
    uint8_t uidLength;  
  
    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {  
      String id = "";  
      for (uint8_t i = 0; i < uidLength; i++) {  
          id += (uid[i] < 0x10 ? "0" : "") + String(uid[i], HEX) + (i == uidLength - 1 ? "" : ":");  
      }  
      Serial.print("UID: ");  
      Serial.println(id);  

      // Adding the Card information the SD Card
      bool status = addRFIDCardToSDCard(name, id);
      if (status){
        sendJsonNotification("OK", RFID, name, id, "RFID Card registered successfully!");
      }else{
        sendJsonNotification("Error", RFID, name, id, "Failed to register RFID card!");
      }
    }

    currentState = RUNNING;  
    Serial.println("Kembali ke mode RUNNING.");
    return;
}

/// delete user NFC function
void deleteUserPN532(String visitorName) {
  bool status = deleteRFIDCardFromSDCard(visitorName);
  // Set the ID null for now, still undecided how this mechanism really work
  String emptyUUID = "00000000-0000-0000-0000-000000000000";

  if (status){
    sendJsonNotification("OK", RFID, visitorName, emptyUUID, "RFID Card deleted successfully!");
  }else{
    sendJsonNotification("Error", RFID, visitorName, emptyUUID, "Failed to delete RFID card!");
  }

  currentState = RUNNING;  
  Serial.println("Kembali ke mode RUNNING.");
  return;
}

/// function pengolahan apakah id dapat mengakses (NFC)
void verifyAccessPN532() {
  uint8_t uid[7];
  uint8_t uidLength;
  unsigned long startTime = millis();
  bool cardDetected = false;

  while (millis() - startTime < 5000) {
    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
      cardDetected = true;
      break;
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }

  if (!cardDetected) {
    Serial.println("Timeout membaca RFID.");
    return;
  }

  String id = "";
  for (uint8_t i = 0; i < uidLength; i++) {
    id += (uid[i] < 0x10 ? "0" : "") + String(uid[i], HEX) + (i == uidLength - 1 ? "" : ":");
  }
  Serial.print("UID: ");
  Serial.println(id);

    File file = SD.open("/data.json", FILE_READ);
    StaticJsonDocument<JSON_CAPACITY> doc;
    if (file) {
      deserializeJson(doc, file);
      file.close();
    }

    bool accessGranted = false;
    String name;
    for (JsonPair kv : doc.as<JsonObject>()) {
      if (kv.value()["uid"] == id) {
        accessGranted = true;
        name = kv.key().c_str();
        break;
      }
    }

    if (accessGranted) {
      Serial.println("Akses Diberikan");
      Serial.print("Nama: ");
      Serial.println(name);
      toggleSolenoid(solenoidCounter);
      delay(500);
    } else {
      Serial.println("Akses Ditolak");
    }
}

/*
>>>> Fingerprint Sensor Function Section <<<<
*/
//Compare fingerprints
void fingerprintMatching(){
  /*Compare the captured fingerprint with all fingerprints in the fingerprint library
    Return fingerprint ID number(1-80) if succeed, return 0 when failed
   */
  uint8_t ret = fingerprint.search();
  if(ret != 0){
    /*Set fingerprint LED ring to always ON in green*/
    fingerprint.ctrlLED(/*LEDMode = */fingerprint.eKeepsOn, /*LEDColor = */fingerprint.eLEDGreen, /*blinkCount = */0);
    Serial.print("Successfully matched,ID=");
    Serial.println(ret);
  }else{
    /*Set fingerprint LED Ring to always ON in red*/
    fingerprint.ctrlLED(/*LEDMode = */fingerprint.eKeepsOn, /*LEDColor = */fingerprint.eLEDRed, /*blinkCount = */0);
    Serial.println("Matching failed");
  }
  delay(1000);
  /*Turn off fingerprint LED Ring*/
  fingerprint.ctrlLED(/*LEDMode = */fingerprint.eNormalClose, /*LEDColor = */fingerprint.eLEDBlue, /*blinkCount = */0);
  Serial.println("-----------------------------");
}

//Fingerprint Registration
void fingerprintRegistration(String visitorName){
  uint8_t ID,i;
  /*Compare the captured fingerprint with all fingerprints in the fingerprint library
    Return fingerprint ID number(1-80) if succeed, return 0 when failed
    Function: clear the last captured fingerprint image
   */
  fingerprint.search(); //Can add "if else" statement to judge whether the fingerprint has been registered. 
  /*Get a unregistered ID for saving fingerprint 
    Return ID number when succeed 
    Return ERR_ID809 if failed
   */
  if((ID = fingerprint.getEmptyID()) == ERR_ID809){
    while(1){
      /*Get error code imformation*/
      //desc = fingerprint.getErrorDescription();
      //Serial.println(desc);
      delay(1000);
    }
  }
  Serial.print("Unregistered ID,ID=");
  Serial.println(ID);
  i = 0;   //Clear sampling times 
  /*Fingerprint Sampling 3 times */
  while(i < COLLECT_NUMBER){
    /*Set fingerprint LED ring to breathing lighting in blue*/
    fingerprint.ctrlLED(/*LEDMode = */fingerprint.eBreathing, /*LEDColor = */fingerprint.eLEDBlue, /*blinkCount = */0);
    Serial.print("The fingerprint sampling of the");
    Serial.print(i+1);
    Serial.println("(th) time is being taken");
    Serial.println("Please press down your finger");
    /*Capture fingerprint image, 10s idle timeout 
      If succeed return 0, otherwise return ERR_ID809
     */
    if((fingerprint.collectionFingerprint(/*timeout = */10)) != ERR_ID809){
      /*Set fingerprint LED ring to quick blink in yellow 3 times*/
      fingerprint.ctrlLED(/*LEDMode = */fingerprint.eFastBlink, /*LEDColor = */fingerprint.eLEDYellow, /*blinkCount = */3);
      Serial.println("Capturing succeeds");
      i++;   //Sampling times +1 
    }else{
      Serial.println("Capturing fails");
      /*Get error code information*/
      //desc = fingerprint.getErrorDescription();
      //Serial.println(desc);
    }
    Serial.println("Please release your finger");
    /*Wait for finger to release
      Return 1 when finger is detected, otherwise return 0 
     */
    while(fingerprint.detectFinger());
  }
  
  /*Save fingerprint information into an unregistered ID*/
  if(fingerprint.storeFingerprint(/*Empty ID = */ID) != ERR_ID809){
    Serial.print("Saving succeed,ID=");
    Serial.println(ID);
    /*Set fingerprint LED ring to always ON in green*/
    fingerprint.ctrlLED(/*LEDMode = */fingerprint.eKeepsOn, /*LEDColor = */fingerprint.eLEDGreen, /*blinkCount = */0);
    delay(1000);
    /*Turn off fingerprint LED ring */
    fingerprint.ctrlLED(/*LEDMode = */fingerprint.eNormalClose, /*LEDColor = */fingerprint.eLEDBlue, /*blinkCount = */0);
  }else{
    Serial.println("Saving failed");
    /*Get error code information*/
    //desc = fingerprint.getErrorDescription();
    //Serial.println(desc);
  }
  Serial.println("-----------------------------");

  Serial.println("------ SAVING FINGERPRINT TO PHYSICAL CARD ------");
  // Save the fingerprint information also in the SD Card
  addFingerprintToSDCard(visitorName, ID);

}

//Fingerprint deletion
void fingerprintDeletion(uint8_t data){
  uint8_t ret;
  /*Compare the captured fingerprint with all the fingerprints in the fingerprint library 
    Return fingerprint ID(1-80) if succeed, return 0 when failed 
   */
  ret = data;
  // ret = fingerprint.search();
  if(ret){
    /*Set fingerprint LED ring to always ON in green*/
    fingerprint.ctrlLED(/*LEDMode = */fingerprint.eKeepsOn, /*LEDColor = */fingerprint.eLEDGreen, /*blinkCount = */0);
    fingerprint.delFingerprint(ret);
    Serial.print("deleted fingerprint,ID=");
    Serial.println(ret);
  }else{
    /*Set fingerprint LED ring to always ON in red*/
    fingerprint.ctrlLED(/*LEDMode = */fingerprint.eKeepsOn, /*LEDColor = */fingerprint.eLEDRed, /*blinkCount = */0);
    Serial.println("Matching failed or the fingerprint hasn't been registered");
  }
  delay(1000);
  /*Turn off fingerprint LED ring*/
  fingerprint.ctrlLED(/*LEDMode = */fingerprint.eNormalClose, /*LEDColor = */fingerprint.eLEDBlue, /*blinkCount = */0);
  Serial.println("-----------------------------");
}

/*
>>>> Notification BLE Section <<<<
*/
void sendJsonNotification(const String& status, LockType type ,const String& name, const String& id, const String& message) {
    // Create a JSON document
    StaticJsonDocument<256> doc;

    // Fill the JSON document
    doc["status"] = status;
    doc["type"] = type;
    JsonObject data = doc.createNestedObject("data");
    data["name"] = name;
    data["id"] = id;
    doc["message"] = message;

    // Serialize the JSON document to a string
    String jsonString;
    serializeJson(doc, jsonString);

    // Set the serialized JSON string as the characteristic value
    pNotification->setValue(jsonString.c_str());

    // Send the notification
    pNotification->notify();

    Serial.println("Notification sent: ");
    Serial.println(jsonString);
}

/*
>>>> Actuator Section <<<<
*/

/// toggle state to Lock/Unlock
void toggleSolenoid(int &counter) {
  counter++;
  if (counter % 2 == 1) {
    Serial.println("Unlocking door...");
    digitalWrite(Solenoid, HIGH);
  } else {
    Serial.println("Locking door...");
    digitalWrite(Solenoid, LOW);
  }
}

/// Setup
void setup() {
  Serial.begin(115200);

  // Inisialiasi Bluetooth Low Energy
  setupBle();

  //Inisialisasi fingerprint dfrobot
  fingerprint.begin();

  // Inisialisasi I2C untuk NFC
  Wire.begin(SDA_PIN, SCL_PIN);
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("Tidak dapat menemukan PN532 board");
    while (true);                                        
  }
  Serial.println("NFC siap");

  // Inisialisasi SPI dan SD card
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
  if (!SD.begin(CS_PIN, SPI)) {
    Serial.println("SD card tidak ditemukan!");
    while (true);                                        
  }
  Serial.println("SD card siap");

  // Inisialisasi UART untuk sensor sidik jari
  // mySerial.begin(57600, SERIAL_8N1, RX_PIN, TX_PIN);
  // if (finger.verifyPassword()) {
  //   Serial.println("Sensor sidik jari ditemukan");
  // } else {
  //   Serial.println("Sensor sidik jari tidak ditemukan");
  //   while (true);                                         
  // }

  pinMode(Solenoid, OUTPUT);
  digitalWrite(Solenoid, LOW);

  // Running task on this program
  xTaskCreate(taskRFID, "RFID Task", 4096, NULL, 1, NULL);
  xTaskCreate(taskFingerprint, "Fingerprint Task", 4096, NULL, 1, NULL);
}

/// Loop
void loop() {
  uint16_t i = 0;
  switch (currentState) {
    case RUNNING:
      if((fingerprint.collectionFingerprint(/*timeout=*/5)) != ERR_ID809){
        fingerprint.ctrlLED(/*LEDMode = */fingerprint.eFastBlink, /*LEDColor = */fingerprint.eLEDBlue, /*blinkCount = */3);  //blue LED blinks quickly 3 times, means it's in fingerprint comparison mode now
        
        /*Wait for finger to relase */
        while(fingerprint.detectFinger()){
          delay(50);
          i++;
          if(i == 5){
            /*Start Set fingerprint LED ring to always ON in yellow color in fingerprint registration mode now*/
            fingerprint.ctrlLED(/*LEDMode = */fingerprint.eFastBlink, /*LEDColor = */fingerprint.eLEDYellow, /*blinkCount = */3);
          }else if(i == 10){
            /*Start Set fingerprint LED ring to always ON in yellow color in fingerprint deletion mode now*/
            fingerprint.ctrlLED(/*LEDMode = */fingerprint.eFastBlink, /*LEDColor = */fingerprint.eLEDRed, /*blinkCount = */3);
          }
        }

        if(i < 5){
          /*Compare fingerprints*/
          Serial.println("Enter fingerprint comparison mode");
          fingerprintMatching();
        }else if(i >= 5 && i < 10){
          /*Registrate fingerprint*/
          Serial.println("Enter fingerprint registration mode");
          fingerprintRegistration(globalJSON["data"]["visitor_name"].as<String>());
        }else{
          /*Delete this fingerprint*/
          Serial.println("Enter fingerprint deletion mode");
          fingerprintDeletion(globalJSON["data"]["visitor_id"].as<uint8_t>());
        }
      }else{
        Serial.println("Fingerprint capturing failed");
      }
      break;
    case ENROLL_RFID:
      enrollUserPN532(globalJSON["data"]["visitor_name"].as<String>());
      delay(50);
      globalJSON.clear();
      currentState = RUNNING;                     // Kembali ke mode normal
      delay(50);
      setup();
      break;
    case DELETE_RFID:
      deleteUserPN532(globalJSON["data"]["visitor_id"].as<String>());
      currentState = RUNNING;   
      globalJSON.clear();
      break;  
    case ENROLL_FP:
      fingerprintRegistration(globalJSON["data"]["name"].as<String>());
      currentState = RUNNING;// Kembali ke mode normal
      globalJSON.clear();
      break;
    case DELETE_FP:
      fingerprintDeletion(globalJSON["data"]["visitor_id"].as<uint8_t>());
      currentState = RUNNING;
      globalJSON.clear();
      break;
    case SYNC_VISITOR:
      Serial.println("kita sync in");
      currentState = RUNNING;
      globalJSON.clear();
      break;
  }
  delay(10);
}

/*
>>>> SD Card Function to Add, Delete, and List Access <<<<
*/
void listFingerprintsFromSDCard(){

}

void addFingerprintToSDCard(String userName, uint8_t fingerprintID) {
  File jsonFile = SD.open("/fingerprints.json", FILE_READ);
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, jsonFile);
  jsonFile.close();

  if (error) {
    Serial.print(F("Failed to read file, error: "));
    Serial.println(error.c_str());
    return;
  }

  if (doc.containsKey(userName)) {
    Serial.println("Username already exists.");
    enrollMode = false;
    return;
  }

  JsonObject user = doc.createNestedObject(userName);
  user["id"] = fingerprintID;

  File writeFile = SD.open("/fingerprints.json", FILE_WRITE);
  if (serializeJson(doc, writeFile) == 0) {
    Serial.println(F("Failed to write to file"));
  } else {
    Serial.println(F("Fingerprint data saved successfully"));
  }
  writeFile.close();
}

void deleteFingerprintFromSDCard(String userName) {
  // Read the JSON file first
  File jsonFile = SD.open("/fingerprints.json", FILE_READ);
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, jsonFile);
  jsonFile.close();

  if (error) {
    Serial.println("Failed to read JSON file.");
    return;
  }

  // Check if the username exists
  if (!doc.containsKey(userName)) {
    Serial.println("Username not found in JSON file.");
    return;
  }

  // Remove the user from the JSON file
  doc.remove(userName);

  // Open the file for writing and save the updated data
  File writeFile = SD.open("/fingerprints.json", FILE_WRITE);
  if (writeFile) {
    serializeJson(doc, writeFile);
    writeFile.close();
    Serial.println("Fingerprint deleted successfully.");
  } else {
    Serial.println("Failed to write JSON file.");
  }

  deleteMode = false;  // Set delete mode to false
}

bool addRFIDCardToSDCard(String username, String RFID){
  File file = SD.open("/data.json", FILE_READ);  
  StaticJsonDocument<JSON_CAPACITY> doc;  
  if (file) {  
    deserializeJson(doc, file);
    file.close();
  }  

  JsonObject user = doc.createNestedObject(username);  
  user["uid"] = RFID;

  file = SD.open("/data.json", FILE_WRITE);  
  if (file) {  
    serializeJson(doc, file);  
    file.close();  
    Serial.println("Data berhasil disimpan.");  
    return true;
  } else {  
    Serial.println("Gagal menyimpan data.");  
    return false;
  }  
}

bool deleteRFIDCardFromSDCard(String data){
  File file = SD.open("/data.json", FILE_READ);
  StaticJsonDocument<JSON_CAPACITY> doc;
  if (file) {
    deserializeJson(doc, file);
    file.close();
  }

  if (doc.containsKey(data)) {
    doc.remove(data);
    Serial.println("Data berhasil dihapus.");
  } else {
    Serial.println("Nama tidak ditemukan.");
  }

  file = SD.open("/data.json", FILE_WRITE);
  if (file) {
    serializeJson(doc, file);
    file.close();
    return true;
  } else {
    Serial.println("Gagal menyimpan data.");
    return false;
  }
}

/*
>>>> Deprecated Function <<<<
*/


/// function to list fingeprint user
// void listAccessFingerprint() {
//   File jsonFile = SD.open("/fingerprints.json", FILE_READ);
//   if (!jsonFile) {
//     Serial.println("Gagal membuka file fingerprints.json.");
//     return;
//   }

//   DynamicJsonDocument doc(1024);
//   DeserializationError error = deserializeJson(doc, jsonFile);
//   jsonFile.close();

//   if (error) {
//     Serial.print("Gagal membaca JSON: ");
//     Serial.println(error.c_str());
//     return;
//   }

//   JsonArray fingerprints = doc["fingerprints"].as<JsonArray>();
//   if (fingerprints.size() == 0) {
//     Serial.println("Tidak ada data sidik jari yang tersimpan.");
//     return;
//   }

//   Serial.println("Daftar Sidik Jari:");
//   for (JsonObject fingerprint : fingerprints) {
//     Serial.print("ID: ");
//     Serial.print(fingerprint["id"].as<int>());
//     Serial.print(", Nama: ");
//     Serial.println(fingerprint["name"].as<String>());
//   }
//   listMode = false;
// }

// /// register UID Fingerprint
// void enrollFingerprint() {
//   Serial.println("Masukkan ID untuk sidik jari (1-127):");
//   while (true) {
//     if (Serial.available()) {
//       id = Serial.parseInt();
//       if (id > 0 && id <= 127) break;
//       Serial.println("ID tidak valid. Gunakan ID antara 1 dan 127:");
//     }
//   }

//   File jsonFile = SD.open("/fingerprints.json", FILE_READ);
//   DynamicJsonDocument doc(1024);
//   DeserializationError error = deserializeJson(doc, jsonFile);
//   jsonFile.close();
  
//   JsonArray fingerprints = doc["fingerprints"].as<JsonArray>();
//   for (JsonObject fingerprint : fingerprints) {
//     if (fingerprint["id"] == id) {
//       Serial.println("ID sudah terdaftar, pilih ID lain.");
//       enrollMode = false;
//       return;
//     }
//   }

//   Serial.println("Masukkan nama pengguna:");
//   String name;
//   while (true) {
//     if (Serial.available()) {
//       name = Serial.readStringUntil('\n');
//       name.trim();
//       if (name.length() > 0) break;
//       Serial.println("Nama tidak boleh kosong. Masukkan nama pengguna:");
//     }
//   }

//   Serial.print("Registrasi sidik jari untuk: ");
//   Serial.print(name);
//   Serial.print(" dengan ID: ");
//   Serial.println(id);
//   Serial.println("Enroll Instruction : Tempelkan sidik jari...");
//   while (finger.getImage() != FINGERPRINT_OK);
//   if (finger.image2Tz(1) != FINGERPRINT_OK) {
//     Serial.println("Gagal mengonversi gambar sidik jari.");
//     return;
//   }
//   Serial.println("Lepaskan sidik jari...");
//   delay(2000);
//   while (finger.getImage() != FINGERPRINT_NOFINGER);
//   Serial.println("Enroll error: Tempelkan sidik jari yang sama...");
//   while (finger.getImage() != FINGERPRINT_OK);
//   if (finger.image2Tz(2) != FINGERPRINT_OK) {
//     Serial.println("Gagal mengonversi gambar sidik jari.");
//     return;
//   }
//   if (finger.createModel() != FINGERPRINT_OK) {
//     Serial.println("Gagal membuat model.");
//     return;
//   }

//   if (finger.storeModel(id) == FINGERPRINT_OK) {
//     Serial.println("Sidik jari berhasil didaftarkan!");

//     // Membaca file JSON yang ada
//     File jsonFile = SD.open("/fingerprints.json", FILE_READ);
//     DynamicJsonDocument doc(1024);
//     DeserializationError error = deserializeJson(doc, jsonFile);
//     jsonFile.close();

//     JsonArray fingerprints = doc["fingerprints"].as<JsonArray>();
//     JsonObject fingerprint = fingerprints.createNestedObject();
//     fingerprint["id"] = id;
//     fingerprint["name"] = name;

//     File writeFile = SD.open("/fingerprints.json", FILE_WRITE);
//     if (writeFile) {
//       serializeJson(doc, writeFile);
//       writeFile.close();
//       Serial.println("Data berhasil disimpan ke file JSON.");
//     } else {
//       Serial.println("Gagal menyimpan data ke file JSON.");
//     }
//   } else {
//     Serial.println("Gagal menyimpan sidik jari.");
//   }
//   enrollMode = false;
//   Serial.println("Kembali ke mode aksi...");
// }

// /// function delete user fingeprint
// void deleteFingerprint() {
//   Serial.println("Masukkan 'ALL' untuk menghapus semua fingerprint atau ID untuk dihapus:");

//   while (true) {
//     if (Serial.available()) {
//       String input = Serial.readStringUntil('\n');
//       input.trim();

//       if (input == "ALL") {
//         Serial.println("Menghapus seluruh data sidik jari...");
//         for (int i = 1; i <= 127; i++) {
//           if (finger.deleteModel(i) == FINGERPRINT_OK) {
//             Serial.print("Fingerprint dengan ID ");
//             Serial.print(i);
//             Serial.println(" berhasil dihapus dari sensor.");
//           } else {
//             Serial.print("Gagal menghapus fingerprint dengan ID ");
//             Serial.println(i);
//           }
//         }

//         File jsonFile = SD.open("/fingerprints.json", FILE_WRITE);
//         if (jsonFile) {
//           jsonFile.print("{\"fingerprints\":[]}");
//           jsonFile.close();
//           Serial.println("Seluruh data berhasil dihapus dari file JSON.");
//         } else {
//           Serial.println("Gagal membuka file fingerprints.json untuk menulis.");
//         }
//         break;

//       } else {
//         int id = input.toInt();
//         if (id > 0 && id <= 127) {
//           File jsonFile = SD.open("/fingerprints.json", FILE_READ);
//           DynamicJsonDocument doc(1024);
//           DeserializationError error = deserializeJson(doc, jsonFile);
//           jsonFile.close();

//           if (error) {
//             Serial.println("Gagal membaca file JSON.");
//             break;
//           }

//           JsonArray fingerprints = doc["fingerprints"].as<JsonArray>();
//           bool found = false;

//           for (JsonArray::iterator it = fingerprints.begin(); it != fingerprints.end(); ++it) {
//             if ((*it)["id"] == id) {
//               fingerprints.remove(it);
//               found = true;
//               break;
//             }
//           }

//           if (found) {
//             if (finger.deleteModel(id) == FINGERPRINT_OK) {
//               Serial.println("Fingerprint berhasil dihapus dari sensor.");
//             } else {
//               Serial.println("Gagal menghapus fingerprint dari sensor.");
//             }

//             File writeFile = SD.open("/fingerprints.json", FILE_WRITE);
//             if (writeFile) {
//               serializeJson(doc, writeFile);
//               writeFile.close();
//               Serial.println("Sidik jari berhasil dihapus dari file JSON.");
//             } else {
//               Serial.println("Gagal menulis file JSON.");
//             }
//           } else {
//             Serial.println("ID tidak ditemukan di file JSON.");
//           }
//           break;
//         } else {
//           Serial.println("ID tidak valid. Gunakan ID antara 1 dan 127 atau 'ALL' untuk menghapus semua.");
//         }
//       }
//     }
//   }
//   deleteMode = false;
// }

// /// function pengolahan apakah id dapat mengakses (fingeprint)
// void actionFingerprint() {
//   Serial.println("Verification : Tempelkan sidik jari...");
//   if (finger.getImage() == FINGERPRINT_OK) {
//     if (finger.image2Tz() == FINGERPRINT_OK && finger.fingerSearch() == FINGERPRINT_OK) {
//       int detectedID = finger.fingerID;
//       String name = "Unknown";
      
//       File jsonFile = SD.open("/fingerprints.json", FILE_READ);
//       if (jsonFile) {
//         DynamicJsonDocument doc(1024);
//         DeserializationError error = deserializeJson(doc, jsonFile);
//         if (!error) {
//           JsonArray fingerprints = doc["fingerprints"].as<JsonArray>();
//           for (JsonObject fingerprint : fingerprints) {
//             if (fingerprint["id"] == detectedID) {
//               name = fingerprint["name"].as<String>();
//               break;
//             }
//           }
//         }
//         jsonFile.close();
//       }

//       Serial.print("Sidik jari cocok. ID: ");
//       Serial.println(detectedID);
//       Serial.print("Nama: ");
//       Serial.println(name);
//       toggleSolenoid(solenoidCounter);
//     } else {
//       Serial.println("Sidik jari tidak dikenali.");
//     }
//   } else {
//     Serial.println("Tidak ada sidik jari yang terdeteksi.");
//   }
// }