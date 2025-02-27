/*

### >>> DEVELOPER NOTE <<< ###
This code provided for TELEMATICS EXPERTISE in door Authentication feature on BATCH#2 Development

HARDWARE PIC            : AHMADHANI FAJAR ADITAMA     github: @
SOFTWARE PIC            : JUNIARTO                    github: @WallNutss 
                          DHIMAS DWI CAHYO            github: @dhimassdwii
MOBILE APP FEATURE PIC  : REZKI MUHAMMAD              github: @rezkim

This microcontroller is used as BLE Server Controlling another ESP32. 
ESP32 included:
  ESP32 Door Control
  ESP32 LED Control (location in dashboard)
  ESP32 AC Control (location middle in front seat)

*/

/// Requirement for Hardware
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <Adafruit_Fingerprint.h>
#include <Adafruit_PN532.h>
#include <esp_log.h>
#include <ArduinoJson.h>

/// Requirement for BLE Connection
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

/// CONFIGURATION PIN GPIO FOR RELAY UNLOCK/LOCK
#define relayUnlock 25    // Relay Unlock
#define relayLock 26      // Relay Lock

/// I2C CONFIGURATION FOR RFID SENSOR
#define SDA_PIN 21            // Pin SDA to PN532
#define SCL_PIN 22            // Pin SCL to PN532

/// UART CONFIGURATION FOR FINGERPRINT SENSOR
#define RX_PIN 16             // Pin RX to TX Fingerprint
#define TX_PIN 17             // Pin TX to RX Fingerprint

/// SPI PIN CONFIGURATION FOR MEMORY CARD MODULE
#define CS_PIN 5        // Chip Select pin
#define SCK_PIN 18      // Clock pin
#define MISO_PIN 19     // Master In Slave Out
#define MOSI_PIN 23     // Master Out Slave In

/// CONSTANT FILE I/O VARIABLE
#define FINGERPRINT_FILE_PATH "/fingerprints.json"
#define RFID_FILE_PATH "/rfid.json"

TaskHandle_t taskRFIDHandle = NULL;
TaskHandle_t taskFingerprintHandle = NULL;

// Enum Device state to define process
enum State {
  RUNNING,
  ENROLL_RFID,
  ENROLL_FP,
  DELETE_RFID,
  DELETE_FP,
  SYNC_VISITOR
};

// Enum Define the lock type
enum LockType {
  RFID,
  FINGERPRINT
};

/// INITIALIZATION VARIABLE
HardwareSerial mySerial(2);              // HardwareSerial
Adafruit_Fingerprint finger(&mySerial);  // Address Fingeprint
Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);    // Address PN532
const size_t JSON_CAPACITY = 1024;       // file JSON
boolean enrollMode = false;              // variabel enrolmode
boolean deleteMode = false;              // variabel deletemode
boolean listMode = false;                // variabel listmode
int id;                                  // variabel ID
bool deviceConnected = false;            // Device connection status
State currentState = RUNNING;            // Initial State
JsonObject globalJSON;

/// Define Macro for Logging Purpose
#define LOG_FUNCTION_LOCAL(message) \
  Serial.println(String(__func__) + ": " + message);

/// Variable Counter and Ouput Relay
int detectionCounter = 0;                 // Counter
bool toggleState = false;                 // Output State

// Function Prototypes
/// Card Sensor (PN532)
void listAccessRFID();
void enrollUserRFID();
void deleteUserRFID();
void verifyAccessRFID();

/// Fingerprint Sensor (R503)
void listFingerprint();
void enrollUserFingerprint();
void deleteUserFingerprint();
void verifyAccessFingerprint();

/// Actuator (Solenoid Doorlock)
void toggleRelay();
void setupRelayDoor();

/// SD Card
void setupSDCard();
bool addFingerprintToSDCard(String username, uint8_t fingerprintID);
bool deleteFingerprintFromSDCard(uint8_t fingerprintToDelete);
bool addRFIDCardToSDCard(String username, String RFID);
bool deleteRFIDCardFromSDCard(String username, String data);

/// Task Controller
void taskRFID(void *parameter);
void taskFingerprint(void *parameter);

/// Setup sensors
void setupPN532();
void setupR503();

/*
>>>> Actuator Section <<<<
*/

/// TOGGLESTATE FOR RELAY UNLOCK/LOCK
void toggleRelay() {
  detectionCounter++;
  if (toggleState) {
    LOG_FUNCTION_LOCAL("relayUnlock ON (DOOR UNLOCK)...");
    digitalWrite(relayUnlock, LOW);                   // Relay UNLOCK ON
    delay(500);
    digitalWrite(relayUnlock, HIGH);                  // Relay UNLOCK OFF
  } else {
    LOG_FUNCTION_LOCAL("relayLock ON (DOOR LOCK)...");
    digitalWrite(relayLock, LOW);                     // Relay LOCK ON
    delay(500);                                       
    digitalWrite(relayLock, HIGH);                    // Relay LOCK OFF
  }
  toggleState = !toggleState;
  LOG_FUNCTION_LOCAL("Total Detections: " + detectionCounter);         
}

void setupRelayDoor() {
  pinMode(relayUnlock, OUTPUT);
  pinMode(relayLock, OUTPUT);
  digitalWrite(relayUnlock, HIGH);
  digitalWrite(relayLock, HIGH);
};


/*
>>>> State Section <<<<
*/

/// TaskRFID Multitasking
void taskRFID(void *parameter) {
  LOG_FUNCTION_LOCAL("Start RFID Reading Task!");
  while (1) {
    if (currentState == RUNNING) {
      verifyAccessRFID();
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

/// Task Fingerprint Multitasking
void taskFingerprint(void *parameter) {
  LOG_FUNCTION_LOCAL("Start Fingerprint Reading Task!");
  while (1) {
    if (currentState == RUNNING) {
      verifyAccessFingerprint();
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

/*
>>>> Sensor Setup Implementation Section <<<<
*/
void setupPN532() {
  Wire.begin(SDA_PIN, SCL_PIN);
  nfc.begin();

  uint32_t versiondata = 0;
  int retries = 1;
  const int maxRetries = 5;
  unsigned long backoffTime = 1000;

  while (retries <= maxRetries) {
    versiondata = nfc.getFirmwareVersion();
    if (versiondata) {
      LOG_FUNCTION_LOCAL("NFC Sensor Ready!");
      return;
    } else {
      LOG_FUNCTION_LOCAL("Cannot find the RFID Sensor PN532! Retrying " + retries + "...");

      retries++;
      delay(backoffTime);

      // Exponential backoff: Double the backoff time after each failure
      backoffTime *= 2;
    }
  }

  // If we reached here, all retries have failed
  LOG_FUNCTION_LOCAL("Failed to initialize NFC Sensor after multiple attempts! Reboot...");
  ESP.restart();
}

void setupR503() {
  mySerial.begin(57600, SERIAL_8N1, RX_PIN, TX_PIN);

  int retries = 1;
  const int maxRetries = 5;
  unsigned long backoffTime = 1000;

  while (retries <= maxRetries) {
    if (finger.verifyPassword()) {
      LOG_FUNCTION_LOCAL("Fingerprint sensor has been found! Proceed");
      return;
    } else {
      LOG_FUNCTION_LOCAL("Fingerprint sensor has not been found! Retrying " + retries + "...");

      retries++;
      delay(backoffTime);

      // Exponential backoff: double the wait time after each failure
      backoffTime *= 2;
    }
  }

  // If we reached here, all retries have failed
  LOG_FUNCTION_LOCAL("Failed to initialize Fingerprint Sensor after multiple attempts! Reboot...");
  ESP.restart();
}

void setupSDCard() {
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);

  int retries = 1;
  const int maxRetries = 5;
  unsigned long backoffTime = 1000;

  while (retries <= maxRetries) {
    if (SD.begin(CS_PIN, SPI)) {
      LOG_FUNCTION_LOCAL("SD Card Ready!");
      return;
    } else {
      LOG_FUNCTION_LOCAL("SD card not found! Retrying " + retries + "...");

      retries++;
      delay(backoffTime);

      // Exponential backoff: double the wait time after each failure
      backoffTime *= 2;
    }
  }

  // If we reached here, all retries have failed
  LOG_FUNCTION_LOCAL("Failed to initialize SD Card after multiple attempts! Reboot...");
  ESP.restart();
}



/*
>>>> NFC Section <<<<
*/
/// Too checking who are registered
void listAccessRFID() {
  File file = SD.open(RFID_FILE_PATH, FILE_READ);
  StaticJsonDocument<JSON_CAPACITY> doc;
  if (file) {
    deserializeJson(doc, file);
    file.close();

    LOG_FUNCTION_LOCAL("Getting List of RFID Access :");
    for (JsonPair kv : doc.as<JsonObject>()) {
      const char *uid = kv.value()["uid"] | "Tidak tersedia";
      LOG_FUNCTION_LOCAL("RFID Data : Name " + kv.key().c_str() + " UID: " + uid);
    }
  } else {
    LOG_FUNCTION_LOCAL("Failed to read the list of RFID Access!");
  }
  return;
}

/// register UID NFC
void enrollUserRFID() {
  LOG_FUNCTION_LOCAL("Enter name for RFID Registration")
  while (Serial.available() == 0);
  String name = Serial.readStringUntil('\n');
  name.trim();

  LOG_FUNCTION_LOCAL("Hold the RFID card close to register (maximum 10s)...")
  uint16_t timeout = 10000;
  String uid_card = gettingRFIDTag(timeout);

  if(uid_card.isEmpty()){
    LOG_FUNCTION_LOCAL("There's no card is detected!");
  }else{
    bool status = addRFIDCardToSDCard(name, uid_card);
  }
}

/// delete user NFC function
void deleteUserRFID() {
  LOG_FUNCTION_LOCAL("Enter username access to be deleted");
  while (Serial.available() == 0);
  String nameToDelete = Serial.readStringUntil('\n');
  nameToDelete.trim();

  LOG_FUNCTION_LOCAL("Hold the RFID card close to register (maximum 3s)...")
  uint16_t timeout = 3000;
  String uid_card = gettingRFIDTag(timeout);

  if(uid_card.isEmpty()){
    LOG_FUNCTION_LOCAL("There's no card is detected! Proceed with not deleting anything");
  }else{
    bool status = deleteRFIDCardFromSDCard(nameToDelete, uid_card);
  }

  LOG_FUNCTION_LOCAL("Back to RUNNING mode.");
  return;
}

/// Verify the access of the NFC/RFID Card
void verifyAccessRFID() {
  bool cardDetected = false;
  uint16_t timeout = 500;
  String uid_card = gettingRFIDTag(timeout);

  if (uid_card.isEmpty()){
    cardDetected = false;
    return;
  }

  bool accessGranted = false;
  if (checkRFIDCardFromSDCard(uid_card)){
    accessGranted = true;
  }

  if (accessGranted) {
    LOG_FUNCTION_LOCAL("Access granteed! Card UID: " + uid_card);
    toggleRelay();
    delay(500);
  }else {
    LOG_FUNCTION_LOCAL("Access denied!");
  }
}

String gettingRFIDTag(uint16_t timeout){
  uint8_t uid[7];
  uint8_t uidLength;
  String uid_card = "";

  bool success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, timeout);
  if (success) {
    for (uint8_t i = 0; i < uidLength; i++) {
      uid_card += (uid[i] < 0x10 ? "0" : "") + String(uid[i], HEX) + (i == uidLength - 1 ? "" : ":");
    }
    LOG_FUNCTION_LOCAL("Found NFC tag with UID: " + uid_card);
  }
  return uid_card;
}


/*
>>>> Fingerprint Sensor Function Section <<<<
*/
/// function to list fingeprint user
void listAccessFingerprint() {
  File jsonFile = SD.open(FINGERPRINT_FILE_PATH, FILE_READ);
  if (!jsonFile) {
    LOG_FUNCTION_LOCAL("Failed to open the file");
    return;
  }

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, jsonFile);
  jsonFile.close();

  if (error) {
    LOG_FUNCTION_LOCAL("Failed to read .json file: " + error.c_str());
    return;
  }

  // Check if the document has any fingerprint data
  if (doc.size() == 0) {
    LOG_FUNCTION_LOCAL("No fingerprints stored in SD Card.");
    return;
  }

  // Listing the fingerprints
  LOG_FUNCTION_LOCAL("Listing Fingerprints:");
  for (JsonPair user : doc.as<JsonObject>()) {
    JsonObject userData = user.value().as<JsonObject>();
    String name = userData["name"].as<String>();
    int id = userData["id"].as<int>();

    LOG_FUNCTION_LOCAL("Name: " + name + " ID: " + String(id));
  }

  listMode = false;
}

/// register UID Fingerprint
void enrollUserFingerprint() {
  LOG_FUNCTION_LOCAL("Entering ID for the Fingerprint Unique ID (1-127):");
  while (true) {
    if (Serial.available()) {
      id = Serial.parseInt();
      if (id > 0 && id <= 127) break;
      LOG_FUNCTION_LOCAL("ID is not Valid!. Please use Fingerprint ID between 1 to 127");
    }
  }

  LOG_FUNCTION_LOCAL("Enter user name: ");
  String name;
  while (true) {
    if (Serial.available()) {
      name = Serial.readStringUntil('\n');
      name.trim();
      if (name.length() > 0) break;
      LOG_FUNCTION_LOCAL("Name cannot be empty! Please input a name!: ");
    }
  }

  LOG_FUNCTION_LOCAL("Fingerprint registration with name  " + name + " and ID " + id);
  LOG_FUNCTION_LOCAL("Waiting for Fingerprint to be pressed...");

  while (finger.getImage() != FINGERPRINT_OK);
  if (finger.image2Tz(1) != FINGERPRINT_OK) {
    LOG_FUNCTION_LOCAL("Failed to convert first Fingerprint image scan!");
    return;
  }

  LOG_FUNCTION_LOCAL("Remove fingerprint from sensor...");
  delay(2000);

  while (finger.getImage() != FINGERPRINT_NOFINGER)
    ;
  LOG_FUNCTION_LOCAL("Please hold the same Fingerprint back to sensor...");
  while (finger.getImage() != FINGERPRINT_OK)
    ;
  if (finger.image2Tz(2) != FINGERPRINT_OK) {
    LOG_FUNCTION_LOCAL("Failed to convert second Fingerprint image scan!");
    return;
  }
  if (finger.createModel() != FINGERPRINT_OK) {
    LOG_FUNCTION_LOCAL("Failed to make the Fingerprint model!");
    return;
  }

  if (finger.storeModel(id) == FINGERPRINT_OK) {
    LOG_FUNCTION_LOCAL("Fingerprint successfully captured with ID " + id);

    bool status = addFingerprintToSDCard(name, id);

  } else {
    LOG_FUNCTION_LOCAL("Failed to stored the fingerprint");
  }
  enrollMode = false;
  LOG_FUNCTION_LOCAL("Back to RUNNING mode.");
  return;
}

/// function delete user fingeprint
void deleteUserFingerprint() {
  LOG_FUNCTION_LOCAL("Enter 'ALL' to delete all of the fingeprint ID:");

  while (true) {
    if (Serial.available()) {
      String input = Serial.readStringUntil('\n');
      input.trim();

      if (input == "ALL") {
        LOG_FUNCTION_LOCAL("Delete all the Fingerprints ID...");
        for (int i = 1; i <= 127; i++) {
          if (finger.deleteModel(i) == FINGERPRINT_OK) {
            LOG_FUNCTION_LOCAL("Fingerprint ID " + i + " successfully deleted from sensor!");
          } else {
            LOG_FUNCTION_LOCAL("Failed to delete Fingerprint ID " + i);
          }
        }

        // TODO : Implement function to delete all the fingerprints when command input is 'ALL'

      } else {
        int id = input.toInt();
        if (id > 0 && id <= 127) {
          File jsonFile = SD.open(FINGERPRINT_FILE_PATH, FILE_READ);
          DynamicJsonDocument doc(1024);
          DeserializationError error = deserializeJson(doc, jsonFile);
          jsonFile.close();

          if (error) {
            LOG_FUNCTION_LOCAL("Failed to read the .json file!");
            break;
          }

          bool found = deleteFingerprintFromSDCard(id);

          if (found) {
            if (finger.deleteModel(id) == FINGERPRINT_OK) {
              LOG_FUNCTION_LOCAL("Fingerprint is successfully deleted from the sensor data!");
            } else {
              LOG_FUNCTION_LOCAL("Failed to delete Fingerprint from the sensor data!");
            }

          } else {
            LOG_FUNCTION_LOCAL("Fingerprint ID is not found on the .json file!");
          }
          break;
        } else {
          LOG_FUNCTION_LOCAL("ID is not valid! Please use ID between 1 to 127 or use 'ALL' to delete all.");
        }
      }
    }
  }
}

/// verify access user fingerprint
void verifyAccessFingerprint() {
  if (finger.getImage() == FINGERPRINT_OK) {
    LOG_FUNCTION_LOCAL("Fingerprint image captured successfully.");
    if (finger.image2Tz() == FINGERPRINT_OK && finger.fingerSearch() == FINGERPRINT_OK) {
      int detectedID = finger.fingerID;
      String name = "Unknown";

      File jsonFile = SD.open(FINGERPRINT_FILE_PATH, FILE_READ);
      if (jsonFile) {
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, jsonFile);
        if (!error) {
          JsonArray fingerprints = doc["fingerprints"].as<JsonArray>();
          for (JsonObject fingerprint : fingerprints) {
            if (fingerprint["id"] == detectedID) {
              name = fingerprint["name"].as<String>();
              break;
            }
          }
        }
        jsonFile.close();
      }

      LOG_FUNCTION_LOCAL("Fingerprint matched!");
      finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_BLUE);
      toggleRelay(); 
      delay(1000);
      finger.LEDcontrol(FINGERPRINT_LED_OFF, 0, FINGERPRINT_LED_BLUE);
    } else {
      finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_RED);
      delay(1000);
      finger.LEDcontrol(FINGERPRINT_LED_OFF, 0, FINGERPRINT_LED_RED);
      LOG_FUNCTION_LOCAL("Fingerprint does not match the stored data. Access denied.");
    }
  }
}

/*
>>>> SD Card Function to Add, Delete, and List Access <<<<
*/
bool addFingerprintToSDCard(String username, uint8_t fingerprintID) {
  LOG_FUNCTION_LOCAL("Saving Fingerprint ID Data, Username: " + username + " ID: " + String(fingerprintID));

  // Checking First
  creatingJsonFile(FINGERPRINT_FILE_PATH);

  File jsonFile = SD.open(FINGERPRINT_FILE_PATH, FILE_READ);
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, jsonFile);
  jsonFile.close();

  if (error && error != DeserializationError::EmptyInput) {
    LOG_FUNCTION_LOCAL("Failed to read file, error: " + error.c_str());
    return false;
  }

  if (error == DeserializationError::EmptyInput) {
    LOG_FUNCTION_LOCAL("Warning! File is Empty");
  }else{
    if (doc.containsKey(username)) {
      LOG_FUNCTION_LOCAL("Username already exists!");
      enrollMode = false;
      return false;
    }

    // Check if any user already has the given ID
    for (JsonPair user : doc.as<JsonObject>()) {
      JsonObject userData = user.value().as<JsonObject>();
      if (userData["id"] == id) {
        LOG_FUNCTION_LOCAL("ID has already been registered! Please choose another Fingerprint ID!");
        enrollMode = false;
        return false;
      }
    }
  }

  JsonObject user = doc.createNestedObject(username);
  user["id"] = fingerprintID;

  File writeFile = SD.open(FINGERPRINT_FILE_PATH, FILE_WRITE);

  if (!writeFile) {
    LOG_FUNCTION_LOCAL(F("Failed to open file for writing"));
    return false;
  }

  if (serializeJson(doc, writeFile) == 0) {
    LOG_FUNCTION_LOCAL(F("Failed to write to file"));
    writeFile.close();
    return false;
  } else {
    LOG_FUNCTION_LOCAL(F("Fingerprint data saved successfully"));
    writeFile.close();
    return true;
  }
}

bool deleteFingerprintFromSDCard(uint8_t fingerprintToDelete) {
  LOG_FUNCTION_LOCAL("Delete Fingerprint ID Data, ID: " + String(fingerprintToDelete));

  // Read the JSON file first
  File jsonFile = SD.open(FINGERPRINT_FILE_PATH, FILE_READ);
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, jsonFile);
  jsonFile.close();

  if (error && error != DeserializationError::EmptyInput) {
    LOG_FUNCTION_LOCAL("Failed to read file, error: " + error.c_str());
    return false;
  }
  
  // Iterate through each key
  bool found = false;
  for (JsonPair user : doc.as<JsonObject>()) {
    JsonObject userData = user.value();

    // Compare the 'id' value for each user
    if (userData.containsKey("id") && userData["id"] == fingerprintToDelete) {
      // Remove the user from the JSON document
      doc.remove(user.key());
      found = true;
      break;
    }
  }

  // If the ID was not found, print a message
  if (!found) {
    LOG_FUNCTION_LOCAL("Fingerprint ID not found in JSON file.");
    return false;
  }

  // Open the file for writing and save the updated data
  deleteMode = false;
  File writeFile = SD.open(FINGERPRINT_FILE_PATH, FILE_WRITE);
  if (writeFile) {
    serializeJson(doc, writeFile);
    writeFile.close();
    LOG_FUNCTION_LOCAL("Fingerprint deleted successfully.");
    return true;
  } else {
    LOG_FUNCTION_LOCAL("Failed to write JSON file.");
    return false;
  }
}

bool addRFIDCardToSDCard(String username, String rfid) {
  LOG_FUNCTION_LOCAL("Saving RFID Data, Username: " + username + " RFID: " + rfid);

  // Checking First
  creatingJsonFile(RFID_FILE_PATH);

  if (checkRFIDCardFromSDCard(rfid)){
    LOG_FUNCTION_LOCAL("RFID " + rfid + " is already registered under another user!");
    return false;
  }

  File file = SD.open(RFID_FILE_PATH, FILE_READ);
  StaticJsonDocument<JSON_CAPACITY> doc;
  if (file) {
    deserializeJson(doc, file);
    file.close();
  }

  // Adding the RFID to the user
  JsonObject user;
  if (doc.containsKey(username)) {
    user = doc[username].as<JsonObject>();
    JsonArray idArray;
    if (user.containsKey("id")) {
        idArray = user["id"].as<JsonArray>();
    } else {
        idArray = user.createNestedArray("id");
    }
    idArray.add(rfid);
    LOG_FUNCTION_LOCAL("Added new UID to existing user: " + username + " RFID: " + rfid);
  } else {
    user = doc.createNestedObject(username);
    JsonArray idArray = user.createNestedArray("id");
    idArray.add(rfid);
    LOG_FUNCTION_LOCAL("Created new user with UID: " + username);
  }

  file = SD.open(RFID_FILE_PATH, FILE_WRITE);
  if (file) {
    serializeJson(doc, file);
    file.close();
    LOG_FUNCTION_LOCAL("RFID data is successfully store to SD Card");
    return true;
  } else {
    LOG_FUNCTION_LOCAL("Failed to store RFID data to SD Card");
    return false;
  }
}

bool deleteRFIDCardFromSDCard(String username, String rfid) {
  LOG_FUNCTION_LOCAL("Delete RFID Data, RFID: " + rfid);

  if (!checkRFIDCardFromSDCard(rfid)){
    LOG_FUNCTION_LOCAL("RFID " + rfid + " is not found in the JSON file!");
    return false;
  }

  File file = SD.open(RFID_FILE_PATH, FILE_READ);
  StaticJsonDocument<JSON_CAPACITY> doc;
  if (file) {
    deserializeJson(doc, file);
    file.close();
  } else {
    LOG_FUNCTION_LOCAL("Failed to open file for reading");
    return false;
  }

  bool found = false;
  if (username.isEmpty()){
    JsonObject userData = doc[username].as<JsonObject>();
    if (userData.containsKey("id")) {
      JsonArray idArray = userData["id"].as<JsonArray>();

      for (int i = 0; i < idArray.size(); i++) {
        if (idArray[i] == rfid) {
          idArray.remove(i);
          found = true;
          LOG_FUNCTION_LOCAL("Removed RFID: " + rfid + " for user: " + username);
          break;
        }
      }

      if (idArray.size() == 0) {
        doc.remove(username);
        LOG_FUNCTION_LOCAL("User " + username + " removed because they have no RFID left!");
      }
    } else {
      LOG_FUNCTION_LOCAL("No 'id' array found for user: " + username);
      return false;
    }
  }else{
    for (JsonPair user : doc.as<JsonObject>()) {
      JsonObject userData = user.value();
      String currentUser = user.key().c_str();

      // Check if the 'id' array exists for this user
      if (userData.containsKey("id")) {
        JsonArray idArray = userData["id"].as<JsonArray>();

        // Iterate through the array and find the RFID to delete
        for (int i = 0; i < idArray.size(); i++) {
          if (idArray[i] == rfid && currentUser == username) {
            idArray.remove(i);
            found = true;
            LOG_FUNCTION_LOCAL("Removed RFID: " + rfid + " for user: " + String(user.key().c_str()));
            break;
          }
        }

        if (idArray.size() == 0) {
          doc.remove(user.key());
          LOG_FUNCTION_LOCAL("User " + String(user.key().c_str()) + " removed because they have no RFID left.");
        }
      }
    }
  }

  if (!found) {
    LOG_FUNCTION_LOCAL("RFID Data not found for Name: " + username + " UID: " + rfid);
    return false;
  }

  file = SD.open(RFID_FILE_PATH, FILE_WRITE);
  if (file) {
    serializeJson(doc, file);
    file.close();
    LOG_FUNCTION_LOCAL("Successfully delete the RFID data!");
    return true;
  } else {
    LOG_FUNCTION_LOCAL("Failed to delete the RFID data!");
    return false;
  }
}

bool checkRFIDCardFromSDCard(String rfid){
  File file = SD.open(RFID_FILE_PATH, FILE_READ);
  StaticJsonDocument<JSON_CAPACITY> doc;
  if (file) {
    deserializeJson(doc, file);
    file.close();
  }

  // Check if this RFID is already been added under some user
  for (JsonPair user : doc.as<JsonObject>()) {
    JsonObject userData = user.value();
    if (userData.containsKey("id")) {
      JsonArray idArray = userData["id"].as<JsonArray>();
      for (int i = 0; i < idArray.size(); i++) {
        if (idArray[i] == rfid) {
          return true;
        }
      }
    }
  }
  return false;
}

void creatingJsonFile(String filePath) {
  // Check if the file exists, if not, create it
  if (!SD.exists(filePath)) {
    LOG_FUNCTION_LOCAL(F("File does not exist, creating a new one."));
    // Open the file in write mode, which will create the file if it doesn't exist
    File newFile = SD.open(filePath, FILE_WRITE);
    // Check if the file is opened successfully
    if (newFile) {
      // Create an empty JSON document
      DynamicJsonDocument doc(1024);

      // Serialize the empty JSON object to the file
      if (serializeJson(doc, newFile) == 0) {
        LOG_FUNCTION_LOCAL(F("Failed to write empty JSON object to the file"));
      } else {
        LOG_FUNCTION_LOCAL(F("Empty JSON object written to file successfully"));
      }
      newFile.close();
    } else {
      LOG_FUNCTION_LOCAL(F("Failed to open file for writing"));
    }
  }
}

/// Setup
void setup() {
  Serial.begin(115200);

  // Initialization of SPI and SD card
  setupSDCard();

  // Initialization of I2C for NFC/RFID sensor
  setupPN532();

  // Initialization of UART for fingerprint sensor
  setupR503();

  // Initialization of relayUnlock and relayLock
  setupRelayDoor();

  // Running task on this program
  xTaskCreate(taskRFID, "RFID Task", 4096, NULL, 1, &taskRFIDHandle);
  xTaskCreate(taskFingerprint, "Fingerprint Task", 4096, NULL, 1, &taskFingerprintHandle);
}

/// Loop
void loop() {
  switch (currentState) {
    case RUNNING:
      if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        if (command == "register_rfid") {
          currentState = ENROLL_RFID;
          vTaskSuspend(taskRFIDHandle);
        } else if (command == "register_fp") {
          currentState = ENROLL_FP;
          vTaskSuspend(taskFingerprintHandle);
        } else if (command == "delete_fp") {
          currentState = DELETE_FP;
          vTaskSuspend(taskFingerprintHandle);
        } else if (command == "delete_rfid") {
          currentState = DELETE_RFID;
          vTaskSuspend(taskRFIDHandle);
        }
      }
      break;

    case ENROLL_RFID:
      LOG_FUNCTION_LOCAL("Start Registering RFID!");
      enrollUserRFID();
      currentState = RUNNING;
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      vTaskResume(taskRFIDHandle);
      break;

    case DELETE_RFID:
      LOG_FUNCTION_LOCAL("Start Deleting RFID!");
      deleteUserRFID();
      currentState = RUNNING;
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      vTaskResume(taskRFIDHandle);
      break;

    case ENROLL_FP:
      LOG_FUNCTION_LOCAL("Start Registering Fingerprint!");
      enrollUserFingerprint();
      currentState = RUNNING;
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      vTaskResume(taskFingerprintHandle);
      break;

    case DELETE_FP:
      LOG_FUNCTION_LOCAL("Start Deleting Fingerprint!");
      deleteUserFingerprint();
      currentState = RUNNING;
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      vTaskResume(taskFingerprintHandle);
      break;
  }
  delay(10);
}
