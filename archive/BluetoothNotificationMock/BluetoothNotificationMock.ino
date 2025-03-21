/*
### >>> DEVELOPER NOTE <<< ###
This code provided for TELEMATICS EXPERTISE
for Mock Up the Bluetooth Communication feature for sending the data response
when action has been done back to Mobile Apps through BLE
*/

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
#define bleServerName "Bluetooth Notification Mock"

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

/// Define Macro for Logging Purpose
#define LOG_FUNCTION_LOCAL(message) \
  Serial.println(String(__func__) + ": " + message);

// Enum Device state to define process
enum State {
  RUNNING,
  GETTING_RFID,
  GETTING_FP,
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
bool deviceConnected = false;            // Device connection status
State currentState = RUNNING;            // Initial State

/// Setup sensors
void setupBLE();

/// Additional handlers for additional characteristic
void handleAcCommand(JsonDocument bleJSON);
void handleLedCommand(JsonDocument bleJSON);

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
      bleKeyAccess = bleJSON["data"]["key_access"].as<String>();

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
 * @param data The JSON object containing the details like lock type, user info, etc.
 * @param message The custom message to include in the notification.
 */
void sendJsonNotification(const String& status, const JsonObject& data, const String& message) {
    LOG_FUNCTION_LOCAL("Sending Notification: Status = " + status + ", Message = " + message);
    
    // Create a JSON document
    StaticJsonDocument<256> doc;

    // Fill the JSON document
    doc["status"] = status;
    doc["data"] = data;
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

void setup() {
  Serial.begin(115200);

  // Initialization of Bluetooth
  setupBLE();
}

void loop() {
  if (bleCommand != "") {
    LOG_FUNCTION_LOCAL("Received Command: " + bleCommand);
    // Process the command received via BLE
    if (bleCommand == "register_fp") {
      StaticJsonDocument<128> dataDoc;
      JsonObject data = dataDoc.to<JsonObject>();
      data["type"] = FINGERPRINT;
      data["name"] = bleName;
      data["key_access"] = String(1);

      sendJsonNotification("OK", data, "Fingerprint registered successfully!");
    }
    if (bleCommand == "register_rfid") {
      StaticJsonDocument<128> dataDoc;
      JsonObject data = dataDoc.to<JsonObject>();
      data["type"] = RFID;
      data["name"] = bleName;
      data["key_access"] = "04:23:5B:6A:00:53:80";
      
      sendJsonNotification("OK", data, "RFID Card registered successfully!");
    }
    if (bleCommand == "delete_rfid") {
      StaticJsonDocument<128> dataDoc;
      JsonObject data = dataDoc.to<JsonObject>();
      data["type"] = RFID;
      data["name"] = bleName;
      data["key_access"] = bleKeyAccess;
      
      sendJsonNotification("OK", data, "RFID Card deleted successfully!");
    }
    if (bleCommand == "delete_fp") {
      StaticJsonDocument<128> dataDoc;
      JsonObject data = dataDoc.to<JsonObject>();
      data["type"] = FINGERPRINT;
      data["name"] = bleName;
      data["key_access"] = bleKeyAccess;

      sendJsonNotification("OK", data, "Fingerprint deleted successfully!");

    }
    if (bleCommand == "update_visitor"){
      const char* incomingData = "{\"data\":{\"RFID\":[{\"name\":\"Jun\",\"key_access\":[\"d5:67:44:37\"]},{\"name\":\"Mas\",\"key_access\":[\"05:83:2b:1c:8b:d1:00\"]},{\"name\":\"ChristopherJonathan\",\"key_access\":[]}],\"FINGERPRINT\":[{\"name\":\"ChristopherJonathan\",\"key_access\":[]}]}}";

      // Deserialize incoming data
      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, incomingData);

      if (error) {
        LOG_FUNCTION_LOCAL("Failed to parse JSON: " + String(error.c_str()));
        return;
      }
      JsonObject data = doc["data"];
      sendJsonNotification("Ok", data, "Success Getting Updated Content!");
    } 
  }

  if (!bleCommand.isEmpty()) {
    bleCommand = "";
    bleName = "";
    bleKeyAccess = "";
  }
  delay(10);
}
