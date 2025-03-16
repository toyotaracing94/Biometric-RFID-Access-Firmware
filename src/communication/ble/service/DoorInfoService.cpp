#include "DoorInfoService.h"
#include <stdio.h>

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
    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Received Data Door Characteristic: Payload = %s", value.c_str());
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

    _pDoorChar = _pService -> createCharacteristic(
        DOOR_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_READ 
    );
    _pDoorChar -> setCallbacks(new DoorCharacteristicCallbacks());
  
    _pACChar = _pService->createCharacteristic(
        AC_REMOTE_CHARACTERISTIC_UUID,  
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_READ 
    );
    _pACChar -> setCallbacks(new ACCharacteristicCallback());

    _pLedChar = _pService->createCharacteristic(
        LED_REMOTE_CHARACTERISTIC_UUID,  
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_READ 
    );
    _pLedChar -> setCallbacks(new LEDRemoteCharacteristicCallback());

    _pService -> start();
}