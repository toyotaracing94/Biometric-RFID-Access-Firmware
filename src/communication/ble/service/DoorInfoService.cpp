#include "DoorInfoService.h"
#include <stdio.h>
#include "entity/CommandBleData.h"
#include "ErrorCodes.h"

void DoorCharacteristicCallbacks::onWrite(BLECharacteristic* pCharacteristic){
    const std::string& value = pCharacteristic -> getValue();
    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Incoming Door Characteristic UUID value of %s", value.c_str());

    JsonDocument incomingData;
    DeserializationError error = deserializeJson(incomingData, value);

    if (error) {
        ESP_LOGE(DOOR_INFO_SERVICE_LOG_TAG, "Failed to deserialize JSON. Error: %s", error.c_str());
        incomingData.clear();
        return;
    }
    
    JsonObject data = incomingData["data"].as<JsonObject>();
    const char* command = incomingData["command"];
    const char* name = data["name"];
    const char* key_access = nullptr;

    if(command != nullptr) ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Received Name: Command = %s", command);

    if(!data.isNull()){
        if(name != nullptr) ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Received Name: Name = %s", name);
        if (data["key_access"].is<const char*>()) {
            key_access = data["key_access"].as<const char*>();
            ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Received Data: Key Access = %s", key_access);
        }
        else if (data["key_access"].is<int>()) {
            int key_access_int = data["key_access"].as<int>();
            char buffer[8];
            itoa(key_access_int, buffer, 10);
            key_access = buffer;
            ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Received Data: Key Access = %s", key_access);
        }
    }

    // Error handling when BLE Door Characteristic Callback kicks in
    if (strcmp(command, "register_fp") == 0) {
        if (name == nullptr){
            char message[20];
            snprintf(message, sizeof(message), "ERROR : %d", FAILED_TO_REGISTER_FINGERPRINT_NO_NAME);
            pCharacteristic -> setValue(message);
            pCharacteristic -> notify();

            ESP_LOGE(DOOR_INFO_SERVICE_LOG_TAG, "Received 'register_fp' command, but 'name' is empty. Cannot proceed.");
            return;
        }
    }
    if (strcmp(command, "delete_fp") == 0) {
        if (name == nullptr && key_access == nullptr){
            char message[20];
            snprintf(message, sizeof(message), "ERROR : %d", FAILED_TO_DELETE_FINGERPRINT_NO_NAME_AND_ID);
            pCharacteristic -> setValue(message);
            pCharacteristic -> notify();

            ESP_LOGE(DOOR_INFO_SERVICE_LOG_TAG, "Received 'delete_fp' command but 'name' and `key access` is empty. Cannot proceed.");
            return;
        }

        if (name == nullptr){
            char message[20];
            snprintf(message, sizeof(message), "ERROR : %d", FAILED_TO_DELETE_FINGERPRINT_NO_NAME);
            pCharacteristic -> setValue(message);
            pCharacteristic -> notify();

            ESP_LOGW(DOOR_INFO_SERVICE_LOG_TAG, "Received 'delete_fp' command but 'name' is empty. Cannot proceed.");
            return;
        }

        if (key_access == nullptr){
            char message[20];
            snprintf(message, sizeof(message), "ERROR : %d", FAILED_TO_DELETE_FINGERPRINT_NO_ID);
            pCharacteristic -> setValue(message);
            pCharacteristic -> notify();

            ESP_LOGW(DOOR_INFO_SERVICE_LOG_TAG, "Received 'delete_fp' command but `key access` is empty. Cannot proceed.");
            return;
        }
    }
    if (strcmp(command, "register_rfid") == 0) {
        if (name == nullptr){
            char message[20];
            snprintf(message, sizeof(message), "ERROR : %d", FAILED_TO_REGISTER_NFC_NO_NAME);
            pCharacteristic -> setValue(message);
            pCharacteristic -> notify();

            ESP_LOGE(DOOR_INFO_SERVICE_LOG_TAG, "Received 'register_rfid' command, but 'name' is empty. Cannot proceed.");
            return;
        }
    }
    if (strcmp(command, "delete_rfid") == 0) {
        if (name == nullptr && key_access == nullptr){
            char message[20];
            snprintf(message, sizeof(message), "ERROR : %d", FAILED_TO_DELETE_NFC_NO_NAME_AND_ID);
            pCharacteristic -> setValue(message);
            pCharacteristic -> notify();

            ESP_LOGE(DOOR_INFO_SERVICE_LOG_TAG, "Received 'delete_rfid' command but 'name' and `key access` is empty. Cannot proceed.");
            return;
        }

        if (name == nullptr){
            char message[20];
            snprintf(message, sizeof(message), "ERROR : %d", FAILED_TO_DELETE_NFC_NO_NAME);
            pCharacteristic -> setValue(message);
            pCharacteristic -> notify();

            ESP_LOGW(DOOR_INFO_SERVICE_LOG_TAG, "Received 'delete_rfid' command but 'name' is empty. Cannot proceed.");
            return;
        }

        if (key_access == nullptr){
            char message[20];
            snprintf(message, sizeof(message), "ERROR : %d", FAILED_TO_DELETE_NFC_NO_ID);
            pCharacteristic -> setValue(message);
            pCharacteristic -> notify();

            ESP_LOGW(DOOR_INFO_SERVICE_LOG_TAG, "Received 'delete_rfid' command but `key access` is empty. Cannot proceed.");
            return;
        }
    }
    
    commandBleData.setCommand(command);
    commandBleData.setName(name);
    commandBleData.setKeyAccess(key_access);
    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Received Valid Data Door Characteristic: Payload = %s", value.c_str());
}

void ACCharacteristicCallback::onWrite(BLECharacteristic* pCharacteristic){
    const std::string& value = pCharacteristic -> getValue();
    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Incoming AC Characteristic UUID value of %s", value.c_str()); 
    pCharacteristic -> setValue(value);
    
    // Maximum for Notify is 20 Bytes Packages Length 
    pCharacteristic -> notify(); 
}

void LEDRemoteCharacteristicCallback::onWrite(BLECharacteristic* pCharacteristic){
    const std::string& value = pCharacteristic -> getValue();
    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Incoming LED Remote Characteristic UUID value of %s", value.c_str());
    pCharacteristic -> setValue(value);

    // Maximum for Notify is 20 Bytes Packages Length 
    pCharacteristic -> notify(); 
}

DoorInfoService::DoorInfoService(BLEServer* pServer) {
    this -> _pServer = pServer;
    this -> _pService = nullptr;
    this -> _pDoorChar = nullptr;
    this -> _pACChar = nullptr;
    this -> _pLedChar = nullptr;
    this -> _pNotificationChar = nullptr;
}

DoorInfoService::~DoorInfoService() {}

void DoorInfoService::startService() {
    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Initializing BLE Door Info Service");
    _pService = _pServer -> createService(SERVICE_UUID);

    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Initializing BLE Door Characteristic");
    _pDoorChar = _pService -> createCharacteristic(
        DOOR_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_READ  |
        BLECharacteristic::PROPERTY_NOTIFY 
    );
    _pDoorChar -> setCallbacks(new DoorCharacteristicCallbacks());
  
    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Initializing BLE AC Remote Characteristic");
    _pACChar = _pService->createCharacteristic(
        AC_REMOTE_CHARACTERISTIC_UUID,  
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_READ 
    );
    _pACChar -> setCallbacks(new ACCharacteristicCallback());

    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Initializing BLE LED Remote Characteristic");
    _pLedChar = _pService->createCharacteristic(
        LED_REMOTE_CHARACTERISTIC_UUID,  
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_READ 
    );
    _pLedChar -> setCallbacks(new LEDRemoteCharacteristicCallback());

    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Initializing BLE Notification Characteristic");
    _pNotificationChar = _pService->createCharacteristic(
        NOTIFICATION_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_NOTIFY | 
        BLECharacteristic::PROPERTY_READ 
    );
    _pNotificationChar->addDescriptor(new BLE2902()); 

    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Add Primary Service for Door Service");
    _pService -> start();
}

void DoorInfoService::sendNotification(JsonDocument& json){
    String buffer;
    serializeJson(json, buffer);
    _pNotificationChar -> setValue(buffer.c_str());
    _pNotificationChar -> notify();
    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Notification sent: %s", buffer.c_str());
}

void DoorInfoService::sendNotification(char* status, char* message){
    JsonDocument document;
    document["status"]  = status;
    document["message"] = message;

    String buffer;
    serializeJson(document, buffer);
    _pNotificationChar -> setValue(buffer.c_str());
    _pNotificationChar -> notify();
    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Notification sent: %s", buffer.c_str());
}