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

/// BLE Characteristic and Configuration
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define LED_REMOTE_CHARACTERISTIC "beb5483e-36e1-4688-b7f5-ea07361b26a8"  
#define AC_REMOTE_CHARACTERISTIC "9f54dbb9-e127-4d80-a1ab-413d48023ad2"  
#define NOTIFICATION_CHARACTERISTIC "01952383-cf1a-705c-8744-2eee6f3f80c8"
#define DOOR_CHARACTERISTIC "ce51316c-d0e1-4ddf-b453-bda6477ee9b9"
#define bleServerName "Yaris Cross Door Auth"

BLEServer* pServer = NULL;                                          // BLE Server
BLEService* pService = NULL;                                        // BLE Service
BLECharacteristic* LEDremoteChar = NULL;                            // LED Characteristic
BLECharacteristic* ACChar = NULL;                                   // AC Characteristic
BLECharacteristic* pNotification = NULL;                            // Notification Characteristic
BLECharacteristic* DoorChar = NULL;                                 // Door Characteristic
BLEAdvertising* pAdvertising = NULL;                                // Device Advertise

/// BLE Command and Data initialization
String bleCommand;
String bleName;
String bleKeyAccess;

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
  RFID = 0,
  FINGERPRINT = 1
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

/// Define Macro for Logging Purpose
#define LOG_FUNCTION_LOCAL(message) \
  Serial.println(String(__func__) + ": " + message);

/// Variable Counter and Ouput Relay
int detectionCounter = 0;                 // Counter
bool toggleState = false;                 // Output State

// Function Prototypes
/// Card Sensor (PN532)
void listAccessRFID();
void enrollUserRFID(String username);
void deleteUserRFID(String username, String uidCard);
void verifyAccessRFID();

/// Fingerprint Sensor (R503)
void listFingerprint();
void enrollUserFingerprint(String username);
void deleteUserFingerprint(String username, String fingerprintId);
void verifyAccessFingerprint();

/// Actuator (Solenoid Doorlock)
void toggleRelay();
void setupRelayDoor();

/// SD Card
void setupSDCard();
std::vector<String> getKeyAccessFromSDCard(String username, bool isFingerprint);
bool addFingerprintToSDCard(String username, uint8_t fingerprintId);
bool deleteFingerprintFromSDCard(String username, uint8_t fingerprintId);
bool addRFIDCardToSDCard(String username, String rfid);
bool deleteRFIDCardFromSDCard(String username, String rfid);

/// Task Controller
void taskRFID(void *parameter);
void taskFingerprint(void *parameter);

/// Setup sensors
void setupBLE();
void setupPN532();
void setupR503();

/// Additional handlers for additional characteristic
void handleAcCommand(JsonDocument bleJSON);
void handleLedCommand(JsonDocument bleJSON);

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


/*
>>>> State Section <<<<
*/

/**
 * @brief The taskRFID function runs the RFID reading task in a loop, continuously runs in the background with a delay between 
 *        iterations to manage the timing of the RFID checks.
 * 
 * This function does not take any input parameters, but it operates in an infinite loop, calling the verifyAccessRFID function
 * when the system's current state is "RUNNING."
 * 
 * The RFID reading process helps manage access control based on RFID tags.
 * 
 * @param parameter A pointer to any additional parameters for the task (not used in this implementation).
 */
void taskRFID(void *parameter) {
  LOG_FUNCTION_LOCAL("Start RFID Reading Task!");
  while (1) {
    if (currentState == RUNNING) {
      verifyAccessRFID();
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

/**
 * @brief The taskFingerprint function runs the Fingerprint sensor reading task in a loop, continuously runs in the background with a delay between 
 *        iterations to manage the timing of the Fingerprint checks.
 * 
 * This function does not take any input parameters, but it operates in an infinite loop, calling the verifyAccessFingerprint function
 * when the system's current state is "RUNNING."
 * 
 * The Fingerprint reading process helps manage access control based on Fingerprint matches.
 * 
 * @param parameter A pointer to any additional parameters for the task (not used in this implementation).
 */
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
>>>> Bluetooth Utilities Section<<<<
*/
/**
 * @brief The MyServerCallbacks class handles BLE server connection events. It manages the connection state and initiates BLE advertising when disconnected.
 * 
 * This class defines two callback functions:
 * - `onConnect`: Called when a device connects to the BLE server, setting the `deviceConnected` flag to true and printing a connection message.
 * - `onDisconnect`: Called when a device disconnects, setting the `deviceConnected` flag to false, printing a disconnection message, and restarting BLE advertising to allow new connections.
 * 
 * These callbacks enable the BLE server to handle connections and disconnections, maintaining proper device connection management.
 */
class MyServerCallbacks : public BLEServerCallbacks {  
  void onConnect(BLEServer* pServer) {  
    deviceConnected = true;  
    LOG_FUNCTION_LOCAL("BLE: connected!");  

  };  
  
  void onDisconnect(BLEServer* pServer) {  
    deviceConnected = false;  
    LOG_FUNCTION_LOCAL("BLE: disconnected!");  
    BLEDevice::startAdvertising();  
  }  
};  

/**
 * @brief Handles BLE characteristic write events and parses the incoming JSON data.
 * 
 * This class listens for write events on a BLE characteristic. Upon receiving data, it attempts to parse the incoming JSON string and extract values such as `command`, `name`, and `keyAccess`. 
 * If the JSON parsing is successful, it logs the received command and data. If parsing fails, an error message is logged.
 */
class CharacteristicCallback : public BLECharacteristicCallbacks {    
  void onWrite(BLECharacteristic* pCharacteristic) {    
    String bleCharacteristicValue = pCharacteristic -> getValue();    
    LOG_FUNCTION_LOCAL("Incoming Characteristic value: " + String(bleCharacteristicValue.c_str()));
    
    // Parse the incoming JSON command  
    JsonDocument bleJSON;
    DeserializationError error = deserializeJson(bleJSON, bleCharacteristicValue);

    if (!error) {
      // I don't know why we can have simply bleData set to bleJSON["data"]
      // Without using this set, the bleData value will simply lost when going on the loop function
      // ArduinoJson is really hard to handle
      JsonObject bleData = bleJSON["data"].as<JsonObject>();
      bleCommand = bleJSON["command"].as<String>();
      bleName = bleJSON["data"]["name"].as<String>();
      bleKeyAccess = bleJSON["data"]["keyAccess"].as<String>();

      String dataStr;
      serializeJson(bleData, dataStr);
      LOG_FUNCTION_LOCAL("Received Data: Command = " + bleCommand + ", Data = " + dataStr);
    } else {
      LOG_FUNCTION_LOCAL("Failed to parse JSON: " + String(error.f_str()));
    }

    // Pass this command to others microcontroller by notifying or broadcast them to this characteristic service
    if (pCharacteristic->getUUID().toString() == LED_REMOTE_CHARACTERISTIC) {  
      handleLedCommand(bleJSON);
    }
    
    if (pCharacteristic->getUUID().toString() == AC_REMOTE_CHARACTERISTIC) {  
      handleAcCommand(bleJSON);
    }

  } 
};

/*
>>>> Notification BLE Section <<<<
*/

/**
 * @brief Sends a JSON notification with status, lock type, user details, and a message.
 * 
 * This function creates a JSON object containing:
 * - The status of the notification (`status`)
 * - The type of lock (`type`)
 * - User information (`name`, `id`)
 * - A custom message (`message`)
 * 
 * The JSON document is serialized and sent as a notification.
 * 
 * @param status The status of the notification (e.g., "success", "failure").
 * @param type The type of lock (enumerated type `LockType`).
 * @param name The name of the user.
 * @param id The identifier associated with the user or RFID.
 * @param message The custom message to include in the notification.
 */
void sendJsonNotification(const String& status, LockType type ,const String& name, const String& id, const String& message) {
    LOG_FUNCTION_LOCAL("Sending Notification: Status = " + status + ", Lock Type = " + type + ", Username = " + name + ", ID = " + id + ", Message = " + message);
    
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

    LOG_FUNCTION_LOCAL("Notification sent: " + jsonString);
}

/// Additional Characteristic Bluetooth Data Handler
/**
 * @brief Handles LED control commands received via Bluetooth.
 * 
 * This function serializes the JSON command for the LED, updates the characteristic value, 
 * and sends a notification to the connected Bluetooth client to update the LED status.
 */
void handleLedCommand(JsonDocument bleJSON) {  
  LOG_FUNCTION_LOCAL("Handling LED command: ");  
  String jsonString;  
  serializeJson(bleJSON, jsonString);
  LEDremoteChar->setValue(jsonString);
  LEDremoteChar->notify();
}


/**
 * @brief Handles AC control commands received via Bluetooth.
 * 
 * This function serializes the JSON command for the AC, updates the characteristic value, 
 * and sends a notification to the connected Bluetooth client to update the AC status.
 */
void handleAcCommand(JsonDocument bleJSON) {  
  LOG_FUNCTION_LOCAL("Handling AC command: ");  
  String jsonString;  
  serializeJson(bleJSON, jsonString);
  ACChar->setValue(jsonString);
  ACChar->notify();
}


/*
>>>> Sensor Setup Implementation Section <<<<
*/
/**
 * @brief Initializes BLE, creates a server, service, and characteristic, and starts advertising.
 * 
 * This function initializes the BLE device, sets up the server, and configures a characteristic for communication. It then starts advertising to allow devices to connect.
 */
void setupBLE() {  
  LOG_FUNCTION_LOCAL("Initializing BLE...");
  BLEDevice::init(bleServerName);  
  
  // Creating the BLE Server for client to connect
  pServer = BLEDevice::createServer();  
  pServer->setCallbacks(new MyServerCallbacks());  
  pService = pServer->createService(SERVICE_UUID);  
  
  // Door Characteristic Service Initialization
  DoorChar = pService->createCharacteristic(  
    DOOR_CHARACTERISTIC,  
    BLECharacteristic::PROPERTY_NOTIFY |  
    BLECharacteristic::PROPERTY_READ |  
    BLECharacteristic::PROPERTY_WRITE);  
  
  DoorChar->addDescriptor(new BLE2902());  
  DoorChar->setCallbacks(new CharacteristicCallback());

  // Notification to Titan Characteristic Service Initialization
  pNotification = pService->createCharacteristic(  
    NOTIFICATION_CHARACTERISTIC,  
    BLECharacteristic::PROPERTY_NOTIFY |  
    BLECharacteristic::PROPERTY_READ |  
    BLECharacteristic::PROPERTY_WRITE);  
  
  pNotification->addDescriptor(new BLE2902());  
  pNotification->setCallbacks(new CharacteristicCallback());

  // LED Remote Characteristic Service Initalization
  LEDremoteChar = pService->createCharacteristic(  
    LED_REMOTE_CHARACTERISTIC,  
    BLECharacteristic::PROPERTY_NOTIFY |  
    BLECharacteristic::PROPERTY_READ |  
    BLECharacteristic::PROPERTY_WRITE);  
  
  LEDremoteChar->addDescriptor(new BLE2902());  
  LEDremoteChar->setCallbacks(new CharacteristicCallback());

  // AC Characteristic Service Initalization
  ACChar = pService->createCharacteristic(  
    AC_REMOTE_CHARACTERISTIC,  
    BLECharacteristic::PROPERTY_NOTIFY |  
    BLECharacteristic::PROPERTY_READ |  
    BLECharacteristic::PROPERTY_WRITE);  
  
  ACChar->addDescriptor(new BLE2902());  
  ACChar->setCallbacks(new CharacteristicCallback());

  // Start Overall Service
  pService->start();  

  // Broadcast the BLE Connection to external
  pAdvertising = BLEDevice::getAdvertising();  
  pAdvertising->addServiceUUID(SERVICE_UUID);  
  pAdvertising->setScanResponse(true);  
  pAdvertising->setMinPreferred(0x06);  
  pAdvertising->setMinPreferred(0x12);  
  BLEDevice::startAdvertising();  
  
  LOG_FUNCTION_LOCAL("BLE initialized. Waiting for client to connect...");  
}  

/**
 * @brief Initializes the PN532 NFC sensor for RFID functionality.
 *        Attempts to initialize the sensor up to 5 times with exponential backoff between retries.
 *        If initialization fails after all retries, the system will restart.
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

/**
 * @brief Initializes the R503 fingerprint sensor.
 *        Attempts to initialize the sensor up to 5 times with exponential backoff between retries.
 *        If initialization fails after all retries, the system will restart.
 */
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

/**
 * @brief Initializes the SD card using SPI interface.
 *        Attempts to initialize the SD card up to 5 times with exponential backoff between retries.
 *        If initialization fails after all retries, the system will restart.
 */
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

/**
 * @brief Initializes Relay connected to Door's output Lock state
 *        Initializes the relay door system by setting up the unlock and lock relays.
 *        Configures the relays as output and sets both relays to the HIGH state (inactive).
 */
void setupRelayDoor() {
  pinMode(relayUnlock, OUTPUT);
  pinMode(relayLock, OUTPUT);
  digitalWrite(relayUnlock, HIGH);
  digitalWrite(relayLock, HIGH);
};


/*
>>>> NFC Section <<<<
*/
/**
 * @brief The higher level function for getting the list of key access of fingereprint under such username
 * 
 * This function takes no function whatsoever at the moment. But a terminal will be run for getting the name
 */
std::vector<String> listAccessRfid() {
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

  std::vector<String> keyAccessList = getKeyAccessFromSDCard(name, false);
  return keyAccessList;
}


/**
 * @brief The higher level function for registering new user RFID Card that will be exposed to external. It will prompts for username if not provided.
 * 
 * @param username The user's name for RFID card registration.
 */
void enrollUserRFID(String username) {
  LOG_FUNCTION_LOCAL("Enroling new RFID User! Username = " + username);

  // Based on this https://forum.arduino.cc/t/null-type/72515/4. But idk why it doesn't hold username == NULL, but instead username == "null" lmao. Weird Arduino String object
  if (username.isEmpty() || username == "null"){
    LOG_FUNCTION_LOCAL("Enter name for RFID Registration")
    while (Serial.available() == 0);
    username = Serial.readStringUntil('\n');
    username.trim();
  }

  LOG_FUNCTION_LOCAL("Hold the RFID card close to register (maximum 10s)...")
  uint16_t timeout = 10000;
  String uid_card = gettingRFIDTag(timeout);

  if(uid_card.isEmpty()){
    LOG_FUNCTION_LOCAL("There's no card is detected!");
  }else{
    bool status = addRFIDCardToSDCard(username, uid_card);
    if (status){
      sendJsonNotification("OK", RFID, username, uid_card, "RFID Card registered successfully!");
    }else{
      sendJsonNotification("Error", RFID, username, uid_card, "Failed to register RFID card!");
    }

  }
}

/**
 * @brief The higher level function for deletes an RFID card for a specified user. It prompts for username and scans the RFID card if not provided.
 * 
 * @param username The user's name for RFID card deletion.
 * @param uidCard The UID of the RFID card to delete. Such format like these (e.g d5:67:44:37)
 */
void deleteUserRFID(String username, String uidCard) {
  LOG_FUNCTION_LOCAL("Deleting RFID User! Username = " + username + " UID = " + uidCard);

  if (username.isEmpty() || username == "null"){
    LOG_FUNCTION_LOCAL("Enter username access to be deleted");
    while (Serial.available() == 0);
    username = Serial.readStringUntil('\n');
    username.trim();
  }

  if (uidCard.isEmpty() || uidCard == "null"){
    LOG_FUNCTION_LOCAL("Hold the RFID card close to register (maximum 5s)...")
    uint16_t timeout = 5000;
    uidCard = gettingRFIDTag(timeout);
  }

  if(uidCard.isEmpty() || uidCard == "null"){
    LOG_FUNCTION_LOCAL("There's no card is detected or ID Card was passed! Proceed with not deleting anything");
  }else{
    bool status = deleteRFIDCardFromSDCard(username, uidCard);
    if (status){
      sendJsonNotification("OK", RFID, username, uidCard, "RFID Card deleted successfully!");
    }else{
      sendJsonNotification("Error", RFID, username, uidCard, "Failed to delete RFID card!");
    }
  }

  LOG_FUNCTION_LOCAL("Back to RUNNING mode.");
  return;
}

/**
 * @brief The higher level function for verifying the access of user RFID Card that will be exposed to external.
 * 
 * This function takes no function whatsoever at the moment
 */
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

/**
 * @brief Function common for getting the UID Card
 * 
 * @param timeout The timeout for the PN532. Do not set this to '0' as this will block all the process!
 * @return The UID Card number. String will be empty or '""' if no card was detected
 */
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
/**
 * @brief The higher level function for getting the list of key access of fingereprint under such username
 * 
 * This function takes no function whatsoever at the moment. But a terminal will be run for getting the name
 */
std::vector<String> listAccessFingerprint() {
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

  std::vector<String> keyAccessList = getKeyAccessFromSDCard(name, true);
  return keyAccessList;
}

/**
 * @brief The higher level function for registering new user Fingerprint that will be exposed to external. It will prompts for username if not provided.
 * 
* @param username The user's name for RFID card registration.
 */
void enrollUserFingerprint(String username) {
  LOG_FUNCTION_LOCAL("Enroling new Fingerprint User! Username = " + username);

  LOG_FUNCTION_LOCAL("Generating ID for the Fingerprint Unique ID (1-256)");
  uint8_t fingerprintId = gettingFingerprintId();

  if (username.isEmpty() || username == "null"){
    LOG_FUNCTION_LOCAL("Enter user name: ");
    while (true) {
      if (Serial.available()) {
        username = Serial.readStringUntil('\n');
        username.trim();
        if (username.length() > 0) break;
        LOG_FUNCTION_LOCAL("Name cannot be empty! Please input a name!: ");
      }
    }
  }

  LOG_FUNCTION_LOCAL("Fingerprint registration with name  " + username + " and Fingerprint ID " + fingerprintId);
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

  if (finger.storeModel(fingerprintId) == FINGERPRINT_OK) {
    LOG_FUNCTION_LOCAL("Fingerprint successfully captured with ID " + fingerprintId);

    bool status = addFingerprintToSDCard(username, fingerprintId);
    if (status){
      sendJsonNotification("OK", FINGERPRINT, username, String(fingerprintId), "Fingerprint registered successfully!");
    }else{
      sendJsonNotification("Error", FINGERPRINT, username, String(fingerprintId), "Failed to register Fingerprint!");
    }

  } else {
    LOG_FUNCTION_LOCAL("Failed to stored the fingerprint");
  }
  enrollMode = false;
  LOG_FUNCTION_LOCAL("Back to RUNNING mode.");
  return;
}


/**
 * @brief The higher level function for deleting user Fingerprint that will be exposed to external. But a terminal will be run for getting the name and also sensor will be run to get the fingerprint image
 * It prompts for username and scans the RFID card if not provided.
 *
 * @param username The user's name for RFID card registration.
 * @param keyAccessFingerprint The Fingerprint ID that was reference to the Fingerprint Image on the Sensor
 */
void deleteUserFingerprint(String username, String keyAccessFingerprint) {
  LOG_FUNCTION_LOCAL("Deleting Fingerprint User! Username = " + username + " FingerprintID = " + keyAccessFingerprint);

  if (keyAccessFingerprint.isEmpty() || keyAccessFingerprint == "null"){
    LOG_FUNCTION_LOCAL("Enter Fingerprint ID to be deleted");
    while (Serial.available() == 0);
    keyAccessFingerprint = Serial.readStringUntil('\n');
    keyAccessFingerprint.trim();
  }
  
  uint8_t fingerprintId = keyAccessFingerprint.toInt();
  
  if (fingerprintId >= 0 && fingerprintId <= 127){
    // Delete all or not. But for now not implemented the all delete
    // TODO : Implement the all delete, this is really hard to abstract as not only we need to delete the fingerprint data on SD Card but also on the sensor
    // TODO : Search for new good ways, for now this will do
    if (fingerprintId == 0) {
      LOG_FUNCTION_LOCAL("Delete all the Fingerprints ID...");
      for (int i = 1; i <= 127; i++) {
        if (finger.deleteModel(i) == FINGERPRINT_OK) {
          LOG_FUNCTION_LOCAL("Fingerprint ID " + i + " successfully deleted from sensor!");
        } else {
          LOG_FUNCTION_LOCAL("Failed to delete Fingerprint ID " + i);
        }
      }
      username = "";
    }else{
      if (username.isEmpty() || username == "null"){
        LOG_FUNCTION_LOCAL("Enter username access to be deleted");
        while (Serial.available() == 0);
        username = Serial.readStringUntil('\n');
        username.trim();
      }
    }
    bool found = deleteFingerprintFromSDCard(username, fingerprintId);

    if (found) {
      if (finger.deleteModel(fingerprintId) == FINGERPRINT_OK) {
        LOG_FUNCTION_LOCAL("Fingerprint is successfully deleted from the sensor data!");
        sendJsonNotification("OK", FINGERPRINT, username, String(fingerprintId), "Fingerprint deleted successfully!");
      } else{
        LOG_FUNCTION_LOCAL("Failed to delete Fingerprint from the sensor data!");
        sendJsonNotification("Error", FINGERPRINT, username, String(fingerprintId), "Failed to delete Fingerprint!");
      }
    }else {
      LOG_FUNCTION_LOCAL("Fingerprint ID is not found on the .json file!");
    }
  }else {
    LOG_FUNCTION_LOCAL("ID is not valid! Please use ID between 1 to 256 or use '0' to delete all.");
  }

}

/**
 * @brief The higher level function for verifying the access of user Fingerprint that will be exposed to external.
 * 
 * This function takes no function whatsoever at the moment
 */
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

/**
 * @brief Function common for getting the Fingerprint ID
 * 
 * @return The Fingerprint ID number
 */
uint8_t gettingFingerprintId(){
  uint8_t fingerprintId;
  while (true) {
    fingerprintId = random(1,20);
    // Check if the generated fingerprint ID exists on the SD card
    if (!checkFingerprintIdFromSDCard(fingerprintId)) {
      // If the ID doesn't exist on the SD card, it's valid
      LOG_FUNCTION_LOCAL("Generated valid fingerprint ID: " + fingerprintId);
      break;
    } else {
      // If the ID exists on the SD card, try again
      LOG_FUNCTION_LOCAL("Fingerprint ID " + fingerprintId + " already exists, generating a new ID...");
    }
  }
  return fingerprintId;
}

/*
>>>> SD Card Function to Add, Delete, and List Access <<<<
*/
/**
 * @brief Return the list of RFID or list of Fingerprint from a user got from the JSON File on the ESP32 System
 * 
 * This function takes an username, and check wether there was list associated
 * if there is will return the list of array of that user access
 * @param username The username
 * @param isFingerprint `true` if getting the list of fingerprint key access, otherwise rfid key access
 * @return list of key access
 */
std::vector<String> getKeyAccessFromSDCard(String username, bool isFingerprint) {
  LOG_FUNCTION_LOCAL("Getting RFID List for User: " + username);
  
  String filePath = isFingerprint ? FINGERPRINT_FILE_PATH : RFID_FILE_PATH;

  // Open the file to read the RFID data
  File file = SD.open(filePath, FILE_READ);
  StaticJsonDocument<JSON_CAPACITY> doc;
  std::vector<String> accessList;

  // If the file exists, read the content
  if (file) {
    deserializeJson(doc, file);
    file.close();

    // Loop through existing users to find the user's RFID cards
    for (JsonObject existingUser : doc.as<JsonArray>()) {
      if (existingUser["name"] == username) {
        JsonArray keyAccessArray = existingUser["key_access"].as<JsonArray>();
        for (JsonVariant v : keyAccessArray) {
          accessList.push_back(v.as<String>());
        }
        break;
      }
    }
  } else {
    LOG_FUNCTION_LOCAL("Failed to open the RFID data file");
  }

  return accessList;
}

/**
 * @brief Add a new Fingerprint access to the SD card.
 * 
 * This function takes a username and Fingerprint ID, and add the corresponding
 * Fingerprint data to the SD card. It returns `true` if the add was successful,
 * or `false` if there was an error or the card was not found.
 * 
 * @param username The username associated with the RFID card. It's required.
 * @param fingerprintID The Fingerprint ID associated with the Fingerprint sensor number to be added. It's required
 * @return `true` if the RFID card(s) were successfully deleted, `false` otherwise.
 */
bool addFingerprintToSDCard(String username, uint8_t fingerprintId) {
  LOG_FUNCTION_LOCAL("Saving Fingerprint ID Data, Username: " + username + " ID: " + String(fingerprintId));

  // Checking First
  creatingJsonFile(FINGERPRINT_FILE_PATH);

  if (checkFingerprintIdFromSDCard(fingerprintId)){
    LOG_FUNCTION_LOCAL("Fingerprint ID " + fingerprintId + " is already registered under another user!");
    return false;
  }

  File jsonFile = SD.open(FINGERPRINT_FILE_PATH, FILE_READ);
  StaticJsonDocument<JSON_CAPACITY> doc;
  
  // Read existing JSON data if file exists
  if (jsonFile) {
    deserializeJson(doc, jsonFile);
    jsonFile.close();
  }

  // Check if we are adding a new user
  bool userFound = false;
  for (JsonObject user : doc.as<JsonArray>()) {
    if (user["name"] == username) {
      JsonArray idArray = user["key_access"].as<JsonArray>();
      idArray.add(fingerprintId);
      userFound = true;
      LOG_FUNCTION_LOCAL("Added new Fingerprint ID to existing user: " + username + " Fingerprint ID: " + fingerprintId);
      break;
    }
  }

  // If user doesn't exist, create a new object for the user
  if (!userFound) {
    JsonObject newUser = doc.createNestedObject();
    newUser["name"] = username;
    JsonArray idArray = newUser.createNestedArray("key_access");
    idArray.add(fingerprintId);
    LOG_FUNCTION_LOCAL("Created new user with Fingerprint ID: " + username);
  }

  // Write the modified JSON data back to the SD card
  jsonFile = SD.open(FINGERPRINT_FILE_PATH, FILE_WRITE);
  if (jsonFile) {
    serializeJson(doc, jsonFile);
    jsonFile.close();
    LOG_FUNCTION_LOCAL("Fingerprint data is successfully stored to SD Card");
    return true;
  } else {
    LOG_FUNCTION_LOCAL("Failed to store Fingerprint data to SD Card");
    return false;
  }
}

/**
 * @brief Deletes the Fingerprint associated with a specific username from the SD card.
 * 
 * This function takes a username and an Fingerprint ID number, and removes the corresponding
 * Fingerprint data from the SD card. It returns `true` if the deletion was successful,
 * or `false` if there was an error or the card was not found.
 * 
 * @param username The username associated with the RFID card. It's required.
 * @param fingerprintId The Fingerprint number to be deleted. If no Fingerprint number is provided (0),
 *             all Fingerprint data under that specified username will be deleted.
 * @return `true` if the Fingerprint(s) were successfully deleted, `false` otherwise.
 */
bool deleteFingerprintFromSDCard(String username, uint8_t fingerprintId) {
  LOG_FUNCTION_LOCAL("Delete Fingerprint ID Data, ID: " + String(fingerprintId));

  // Read the JSON file first
  File file = SD.open(FINGERPRINT_FILE_PATH, FILE_READ);
  StaticJsonDocument<JSON_CAPACITY> doc;
  if (!file) {
    LOG_FUNCTION_LOCAL("Failed to open file for reading");
    return false;
  }

  deserializeJson(doc, file);
  file.close();

  bool found = false;
  bool userFound = false;
  
  for (JsonObject user : doc.as<JsonArray>()) {
    String currentUser = user["name"].as<String>();

    // Check if the current user matches the provided username and not delete all the fingerprint ID
    if (fingerprintId != 0 && currentUser == username) {
      userFound = true;
      if (user.containsKey("key_access")) {
        JsonArray fingerprintArray = user["key_access"].as<JsonArray>();
        // If a specific Fingerprint is provided, search for it and remove it
        for (int i = 0; i < fingerprintArray.size(); i++) {
          if (fingerprintArray[i].as<uint8_t>() == fingerprintId) {
            fingerprintArray.remove(i);
            found = true;
            LOG_FUNCTION_LOCAL("Removed Fingerprint ID: " + fingerprintId + " for user: " + currentUser);
            break;
          }
        }
      }
    }else{ // Will delete all if the fingerprintId == 0
      // If no Fingerprint is provided or 0, just clear all
      userFound = true;
      found = true;
      user.clear();
      LOG_FUNCTION_LOCAL("All Fingerprints Access removed for user: " + username);
    }
  }

  // If the user was not found, log and return false
  if (!userFound) {
    LOG_FUNCTION_LOCAL("Username " + username + " not found.");
    return false;
  }

  // If no RFID was found to remove, log and return false
  if (!found) {
    LOG_FUNCTION_LOCAL("Fingerprint ID " + fingerprintId + " not found for deletion.");
    return false;
  }

  // Write the modified data back to the file
  file = SD.open(FINGERPRINT_FILE_PATH, FILE_WRITE);
  if (file) {
    serializeJson(doc, file);
    file.close();
    LOG_FUNCTION_LOCAL("Fingerprint data successfully updated on SD Card.");
    return true;
  } else {
    LOG_FUNCTION_LOCAL("Failed to open Fingerprint file for writing!");
    return false;
  }
}

/**
 * @brief Checking does Fingerprint ID already registered on the ESP32 system which is on the json file at then moment
 * 
 * This function takes an Fingerprint ID number, and check wether there was and Fingerprint ID already registered or not . It returns `true` if the data is registered,
 * or `false` if the data is not registered on the ESP32
 * 
 * @param rfid The RFID number to be checked
 * @return `true` if the RFID card(s) were already registered, `false`.
 */
bool checkFingerprintIdFromSDCard(uint8_t fingerprintId) {
  LOG_FUNCTION_LOCAL("Checking Fingerprint ID: " + String(fingerprintId));

  File jsonFile = SD.open(FINGERPRINT_FILE_PATH, FILE_READ);
  if (!jsonFile) {
    LOG_FUNCTION_LOCAL("Failed to open fingerprint data file for reading!");
    return false;
  }

  StaticJsonDocument<JSON_CAPACITY> doc;
  DeserializationError error = deserializeJson(doc, jsonFile);
  jsonFile.close();
  if (error) {
    LOG_FUNCTION_LOCAL("Failed to parse JSON file!");
    return false;
  }

  // Loop through the array of users
  for (JsonObject user : doc.as<JsonArray>()) {
    // Check if the fingerprintId array exists
    if (user.containsKey("key_access")) {
      JsonArray idArray = user["key_access"].as<JsonArray>();

      // Loop through the fingerprintId array to check for the given ID
      for (int i = 0; i < idArray.size(); ++i) {
        if (idArray[i].as<uint8_t>() == fingerprintId) {
          LOG_FUNCTION_LOCAL("Fingerprint ID " + String(fingerprintId) + " found in user: " + user["name"].as<String>());
          return true;
        }
      }
    }
  }

  LOG_FUNCTION_LOCAL("Fingerprint ID " + String(fingerprintId) + " not found in any user.");
  return false;
}

/**
 * @brief Add a new RFID card to the SD card.
 * 
 * This function takes a username and an RFID number get from the RFID sensor, and add the corresponding
 * RFID card data to the SD card. It returns `true` if the add was successful,
 * or `false` if there was an error or the card was not found.
 * 
 * @param username The username associated with the RFID card. It's required.
 * @param rfid The RFID number to be added. It's required
 * @return `true` if the RFID card(s) were successfully deleted, `false` otherwise.
 */
bool addRFIDCardToSDCard(String username, String rfid) {
  LOG_FUNCTION_LOCAL("Saving RFID Data, Username: " + username + " RFID: " + rfid);

  // Checking First
  creatingJsonFile(RFID_FILE_PATH);

  // Prevent rfid to be registered under two same account user 
  if (checkRFIDCardFromSDCard(rfid)) {
    LOG_FUNCTION_LOCAL("RFID " + rfid + " is already registered under another user!");
    return false;
  }

  File file = SD.open(RFID_FILE_PATH, FILE_READ);
  StaticJsonDocument<JSON_CAPACITY> doc;

  // If the file exists, read the content
  if (file) {
    deserializeJson(doc, file);
    file.close();
  }

  // Look for the user and add RFID to the user's array
  JsonObject user;
  bool userFound = false;

  // Loop through existing users to find the one to update
  for (JsonObject existingUser : doc.as<JsonArray>()) {
    if (existingUser["name"] == username) {
      JsonArray rfidArray = existingUser["key_access"].as<JsonArray>();
      rfidArray.add(rfid);
      userFound = true;
      LOG_FUNCTION_LOCAL("Added RFID to existing user: " + username + " RFID: " + rfid);
      break;
    }
  }

  // If the user doesn't exist, create a new user entry
  if (!userFound) {
    JsonObject newUser = doc.createNestedObject();
    newUser["name"] = username;
    JsonArray rfidArray = newUser.createNestedArray("key_access");
    rfidArray.add(rfid);
    LOG_FUNCTION_LOCAL("Created new user with RFID: " + username);
  }

  // Reopen the file for writing the updated JSON
  file = SD.open(RFID_FILE_PATH, FILE_WRITE);
  if (file) {
    serializeJson(doc, file);
    file.close();
    LOG_FUNCTION_LOCAL("RFID data is successfully stored to SD Card");
    return true;
  } else {
    LOG_FUNCTION_LOCAL("Failed to store RFID data to SD Card");
    return false;
  }
}

/**
 * @brief Deletes the RFID card associated with a specific username from the SD card.
 * 
 * This function takes a username and an RFID number, and removes the corresponding
 * RFID card data from the SD card. It returns `true` if the deletion was successful,
 * or `false` if there was an error or the card was not found.
 * 
 * @param username The username associated with the RFID card. It's required.
 * @param rfid The RFID number to be deleted. If no RFID number is provided (empty string),
 *             all RFID cards under the specified username will be deleted.
 * @return `true` if the RFID card(s) were successfully deleted, `false` otherwise.
 */
bool deleteRFIDCardFromSDCard(String username, String rfid) {
  LOG_FUNCTION_LOCAL("Delete RFID Card Data, Username: " + username + ", RFID: " + rfid);

  File file = SD.open(RFID_FILE_PATH, FILE_READ);
  StaticJsonDocument<JSON_CAPACITY> doc;
  if (!file) {
    LOG_FUNCTION_LOCAL("Failed to open file for reading");
    return false;
  }

  deserializeJson(doc, file);
  file.close();

  bool found = false;
  bool userFound = false;

  for (JsonObject user : doc.as<JsonArray>()) {
    String currentUser = user["name"].as<String>();

    // Check if the current user matches the provided username
    if (currentUser == username) {
      userFound = true;
      
      if (user.containsKey("key_access")) {
        JsonArray rfidArray = user["key_access"].as<JsonArray>();

        if (rfid.isEmpty()) {
          // If no RFID is provided, remove all RFID cards for the user
          rfidArray.clear();
          found = true;
          LOG_FUNCTION_LOCAL("All RFID cards removed for user: " + username);
        } else {
          // If a specific RFID is provided, search for it and remove it
          for (int i = 0; i < rfidArray.size(); i++) {
            if (rfidArray[i].as<String>() == rfid) {
              rfidArray.remove(i);
              found = true;
              LOG_FUNCTION_LOCAL("Removed RFID: " + rfid + " for user: " + currentUser);
              break;
            }
          }
        }

        // If the user has no RFID cards left, remove the user entirely
        // TODO : Implement this, I still don't get it how we can do this

      }
    }
  }

  // If the user was not found, log and return false
  if (!userFound) {
    LOG_FUNCTION_LOCAL("Username " + username + " not found.");
    return false;
  }

  // If no RFID was found to remove, log and return false
  if (!found) {
    LOG_FUNCTION_LOCAL("RFID " + rfid + " not found for deletion.");
    return false;
  }

  // Write the modified data back to the file
  file = SD.open(RFID_FILE_PATH, FILE_WRITE);
  if (file) {
    serializeJson(doc, file);
    file.close();
    LOG_FUNCTION_LOCAL("RFID data successfully updated on SD Card.");
    return true;
  } else {
    LOG_FUNCTION_LOCAL("Failed to open RFID file for writing!");
    return false;
  }
}

/**
 * @brief Checking does RFID already registered on the ESP32 system which is on the json file at then moment
 * 
 * This function takes an RFID number, and check wether there was and RFID already registered or not . It returns `true` if the data is registered,
 * or `false` if the data is not registered on the ESP32
 * 
 * @param rfid The RFID number to be checked
 * @return `true` if the RFID card(s) were already registered, `false`.
 */
bool checkRFIDCardFromSDCard(String rfid) {
  // Open the file for reading
  File file = SD.open(RFID_FILE_PATH, FILE_READ);
  StaticJsonDocument<JSON_CAPACITY> doc;

  // If the file exists, deserialize the content
  if (!file) {
    LOG_FUNCTION_LOCAL("Failed to open the file for reading");
    return false;
  }

  deserializeJson(doc, file);
    file.close();

  // Loop through all users to check for the RFID
  for (JsonObject user : doc.as<JsonArray>()) {
    if (user.containsKey("key_access")) {
      JsonArray rfidArray = user["key_access"].as<JsonArray>();

      // Check if the RFID is in the user's rfid array
      for (int i = 0; i < rfidArray.size(); i++) {
        if (rfidArray[i].as<String>() == rfid) {
          LOG_FUNCTION_LOCAL("RFID " + rfid + " found under user: " + user["name"].as<String>());
          return true;
        }
      }
    }
  }

  // Return false if RFID is not found
  LOG_FUNCTION_LOCAL("RFID " + rfid + " not found in the SD card.");
  return false;
}

/**
 * @brief Creating Initial JSON if the file is not already initialized on the 
 * 
 * @param filePath The filePath of the JSON File need to be initiated
 */
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

  // Initialization of Bluetooth
  setupBLE();

  // Running task on this program
  xTaskCreate(taskRFID, "RFID Task", 4096, NULL, 1, &taskRFIDHandle);
  xTaskCreate(taskFingerprint, "Fingerprint Task", 4096, NULL, 1, &taskFingerprintHandle);
}

/// Loop
void loop() {
  bool hasExecuted = false;

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

      if (bleCommand != "") {
        LOG_FUNCTION_LOCAL("Received Command: " + bleCommand);
        // Process the command received via BLE
        if (bleCommand == "register_fp") {
          currentState = ENROLL_FP;
          vTaskSuspend(taskFingerprintHandle);
        }
        else if (bleCommand == "register_rfid") {
          currentState = ENROLL_RFID;
          vTaskSuspend(taskRFIDHandle);
        }
        else if (bleCommand == "delete_rfid") {
          currentState = DELETE_RFID;
          vTaskSuspend(taskRFIDHandle); 
        }
        else if (bleCommand == "delete_fp") {
          currentState = DELETE_FP;
          vTaskSuspend(taskFingerprintHandle);
        }
      }
      break;

    case ENROLL_RFID:
      LOG_FUNCTION_LOCAL("Start Registering RFID!");
      enrollUserRFID(bleName);

      currentState = RUNNING;
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      vTaskResume(taskRFIDHandle);
      hasExecuted = true;
      break;

    case DELETE_RFID:
      LOG_FUNCTION_LOCAL("Start Deleting RFID!");
      deleteUserRFID(bleName, bleKeyAccess);

      currentState = RUNNING;
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      vTaskResume(taskRFIDHandle);
      hasExecuted = true;
      break;

    case ENROLL_FP:
      LOG_FUNCTION_LOCAL("Start Registering Fingerprint!");
      enrollUserFingerprint(bleName);

      currentState = RUNNING;
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      vTaskResume(taskFingerprintHandle);
      hasExecuted = true;
      break;

    case DELETE_FP:
      LOG_FUNCTION_LOCAL("Start Deleting Fingerprint!");
      deleteUserFingerprint(bleName, bleKeyAccess);

      currentState = RUNNING;
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      vTaskResume(taskFingerprintHandle);
      hasExecuted = true;
      break;
  }
  // Clear the command of the ble because using global, it should have not do this, but I don't know any other method for now
  // TODO : Search for a way to incoporate the BLE callback to the loop without using global function
  if (hasExecuted && !bleCommand.isEmpty()) {
    bleCommand = "";
    bleName = "";
    bleKeyAccess = "";
  }
  delay(10);
}
