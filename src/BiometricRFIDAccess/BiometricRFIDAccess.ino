/*
### >>> DEVELOPER NOTE <<< ###
This code provided for TELEMATICS EXPERTISE in door Authentication feature on BATCH#2 Development

HARDWARE PIC : AHMADHANI FAJAR ADITAMA 
SOFTWARE PIC : DHIMAS DWI CAHYO
MOBILE APP FEATURE PIC : REZKI MUHAMMAD github: @rezkim

This microcontroller is used as BLE Server Controlling another ESP32. 
ESP32 included :
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

/// CONSTANT VARIABLE
#define Solenoid1 25  // Pin Solenoid (prototype)
#define Solenoid2 26
#define SDA_PIN 21   // Pin SDA to PN532
#define SCL_PIN 22   // Pin SCL to PN532
#define RX_PIN 16    // Pin RX to TX Fingerprint
#define TX_PIN 17    // Pin TX to RX Fingerprint

#define CS_PIN 5     // Chip Select pin
#define SCK_PIN 18   // Clock pin
#define MISO_PIN 19  // Master In Slave Out
#define MOSI_PIN 23  // Master Out Slave In

/// CONSTANT FILE I/O VARIABLE
#define FINGERPRINT_FILE_PATH "/fingerprints.json"
#define RFID_FILE_PATH "/data.json"

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

int id;                        // variabel ID
int solenoidCounter = 0;       // counter Solenoid
bool deviceConnected = false;  // Device connection status
State currentState = RUNNING;  // Initial State
JsonObject globalJSON;

/// Define Macro for Logging Purpose
#define LOG_FUNCTION_LOCAL(message) \
  Serial.println(String(__func__) + ": " + message);

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
void toggleSolenoid(int &counter);
void setupSolenoid();

/// SD Card
void setupSDCard();
bool addFingerprintToSDCard(String username, uint8_t fingerprintID);
bool deleteFingerprintFromSDCard(uint8_t fingerprintToDelete);
bool addRFIDCardToSDCard(String username, String RFID);
bool deleteRFIDCardFromSDCard(String data);

/// Task Controller
void taskRFID(void *parameter);
void taskFingerprint(void *parameter);

/// Setup sensors
void setupPN532();
void setupR503();

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

  LOG_FUNCTION_LOCAL("Hold the RFID card close to register...")
  uint8_t uid[7];
  uint8_t uidLength;

  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
    String id = "";
    for (uint8_t i = 0; i < uidLength; i++) {
      id += (uid[i] < 0x10 ? "0" : "") + String(uid[i], HEX) + (i == uidLength - 1 ? "" : ":");
    }
    LOG_FUNCTION_LOCAL("UID " + id);

    // Adding the Card information the SD Card
    bool status = addRFIDCardToSDCard(name, id);
  }

  currentState = RUNNING;
  LOG_FUNCTION_LOCAL("Back to RUNNING mode.");
  return;
}

/// delete user NFC function
void deleteUserRFID() {
  LOG_FUNCTION_LOCAL("Enter username access to be deleted");
  while (Serial.available() == 0)
    ;
  String nameToDelete = Serial.readStringUntil('\n');
  nameToDelete.trim();

  bool status = deleteRFIDCardFromSDCard(nameToDelete);

  currentState = RUNNING;
  LOG_FUNCTION_LOCAL("Back to RUNNING mode.");
  return;
}

/// Verify the access of the NFC/RFID Card
void verifyAccessRFID() {
  LOG_FUNCTION_LOCAL("Put your Card in the Sensor...");
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
    LOG_FUNCTION_LOCAL("Timeout reading RFID.");
    return;
  }

  String id = "";
  for (uint8_t i = 0; i < uidLength; i++) {
    id += (uid[i] < 0x10 ? "0" : "") + String(uid[i], HEX) + (i == uidLength - 1 ? "" : ":");
  }
  LOG_FUNCTION_LOCAL("UID " + id);

  File file = SD.open(RFID_FILE_PATH, FILE_READ);
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
    LOG_FUNCTION_LOCAL("Access granteed");
    LOG_FUNCTION_LOCAL("Name " + name);
    toggleSolenoid(solenoidCounter);
    delay(500);
  } else {
    LOG_FUNCTION_LOCAL("Access denied!");
  }
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

  // Check whether the fingerprint already exists before even reading them
  File jsonFile = SD.open(FINGERPRINT_FILE_PATH, FILE_READ);
  if (!jsonFile) {
    LOG_FUNCTION_LOCAL("Failed to open the file.");
    return;
  }

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, jsonFile);
  jsonFile.close();

  if (error) {
    LOG_FUNCTION_LOCAL("Failed to read .json file: " + error.c_str());
    return;
  }

  // Check if any user already has the given ID
  for (JsonPair user : doc.as<JsonObject>()) {
    JsonObject userData = user.value().as<JsonObject>();
    if (userData["id"] == id) {
      LOG_FUNCTION_LOCAL("ID has already been registered! Please choose another Fingerprint ID!");
      enrollMode = false;
      return;
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

  while (finger.getImage() != FINGERPRINT_OK)
    ;
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
  LOG_FUNCTION_LOCAL("Press your Fingerprint...");
  if (finger.getImage() == FINGERPRINT_OK) {
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
      toggleSolenoid(solenoidCounter);

      delay(1000);
      finger.LEDcontrol(FINGERPRINT_LED_OFF, 0, FINGERPRINT_LED_BLUE);
    } else {
      finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_RED);
      delay(1000);
      finger.LEDcontrol(FINGERPRINT_LED_OFF, 0, FINGERPRINT_LED_RED);
      LOG_FUNCTION_LOCAL("Fingerprint is not really.");
    }
  } else {
    LOG_FUNCTION_LOCAL("There is no fingerprints are detected!");
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
  }

  if (doc.containsKey(username)) {
    LOG_FUNCTION_LOCAL("Username already exists!");
    enrollMode = false;
    return false;
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

  // Checking First
  creatingJsonFile(FINGERPRINT_FILE_PATH);

  // Read the JSON file first
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

bool addRFIDCardToSDCard(String username, String RFID) {
  LOG_FUNCTION_LOCAL("Saving RFID Data, Username: " + username + " RFID: " + RFID);

  // Checking First
  creatingJsonFile(RFID_FILE_PATH);

  File file = SD.open(RFID_FILE_PATH, FILE_READ);
  StaticJsonDocument<JSON_CAPACITY> doc;
  if (file) {
    deserializeJson(doc, file);
    file.close();
  }

  JsonObject user = doc.createNestedObject(username);
  user["uid"] = RFID;

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

bool deleteRFIDCardFromSDCard(String data) {
  LOG_FUNCTION_LOCAL("Delete RFID Data, RFID: " + data);

  // Checking First
  creatingJsonFile(RFID_FILE_PATH);

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
  for (JsonPair user : doc.as<JsonObject>()) {
    JsonObject userData = user.value();

    // Compare the 'id' value for each user
    if (userData.containsKey("uid") && userData["uid"] == data) {
      // Remove the user from the JSON document
      doc.remove(user.key());
      found = true;
      break;
    }
  }

  if (!found) {
    LOG_FUNCTION_LOCAL("RFID Data not found for RFID: " + RFID);
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


/*
>>>> Actuator Section <<<<
*/

/// toggle state to Lock/Unlock
void toggleSolenoid(int &counter) {
  counter++;
  if (counter % 2 == 1) {
    LOG_FUNCTION_LOCAL("Unlocking door...");
    digitalWrite(Solenoid, HIGH);
  } else {
    LOG_FUNCTION_LOCAL("Locking door...");
    digitalWrite(Solenoid, LOW);
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

  // Trial solenoid
  pinMode(Solenoid, OUTPUT);
  digitalWrite(Solenoid, LOW);

  // // Running task on this program
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
