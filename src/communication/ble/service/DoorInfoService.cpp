#include "DoorInfoService.h"
#include <stdio.h>
#include "entity/CommandBleData.h"
#include "StatusCodes.h"

/**
 * @brief BLE callback class for handling write operations on the Door characteristic.
 * 
 */
void DoorCharacteristicCallbacks::onWrite(BLECharacteristic* pCharacteristic){
    const std::string& value = pCharacteristic -> getValue();
    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Incoming Door Characteristic UUID value of %s", value.c_str());

    JsonDocument incomingData;
    DeserializationError error = deserializeJson(incomingData, value);

    if (error)
    {
        ESP_LOGE(DOOR_INFO_SERVICE_LOG_TAG, "Failed to deserialize JSON. Error: %s", error.c_str());
        incomingData.clear();
        return;
    }

    JsonObject data = incomingData["data"].as<JsonObject>();
    const char *command = incomingData["command"];
    const char *name = data["name"];
    const char *key_access = nullptr;
    const char *visitor_id = nullptr;

    if (command != nullptr)
        ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Received Name: Command = %s", command);

    if (!data.isNull())
    {
        if (name != nullptr)
            ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Received Name: Name = %s", name);
        if (data["key_access"].is<const char *>())
        {
            key_access = data["key_access"].as<const char *>();
            ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Received Data: Key Access = %s", key_access);
        }
        else if (data["key_access"].is<int>())
        {
            int key_access_int = data["key_access"].as<int>();
            char buffer[8];
            itoa(key_access_int, buffer, 10);
            key_access = buffer;
            ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Received Data: Key Access = %s", key_access);
        }

        if (data["visitor_id"].is<const char *>())
        {

            visitor_id = data["visitor_id"].as<const char *>();
            ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Received Data: Visitor ID = %s", visitor_id);
        }
        else if (data["visitor_id"].is<int>())
        {
            int visitor_id_int = data["visitor_id"].as<int>();
            char buffer[8];
            itoa(visitor_id_int, buffer, 10);
            visitor_id = buffer;
            ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Received Data: Visitor ID = %s", visitor_id);
        }
    }

    // Error handling when BLE Door Characteristic Callback kicks in
    if (strcmp(command, "register_fp") == 0)
    {
        if (name == nullptr)
        {
            char message[20];
            snprintf(message, sizeof(message), "ERROR : %d", FAILED_TO_REGISTER_FINGERPRINT_NO_NAME);
            pCharacteristic->setValue(message);
            pCharacteristic->notify();

            ESP_LOGE(DOOR_INFO_SERVICE_LOG_TAG, "Received 'register_fp' command, but 'name' is empty. Cannot proceed.");
            return;
        }
    }
    if (strcmp(command, "delete_fp") == 0)
    {

        if (visitor_id == nullptr)
        {
            char message[20];
            snprintf(message, sizeof(message), "ERROR : %d", FAILED_TO_DELETE_FINGERPRINT_NO_ID);
            pCharacteristic->setValue(message);
            pCharacteristic->notify();

            ESP_LOGW(DOOR_INFO_SERVICE_LOG_TAG, "Received 'delete_fp' command but `visitor_id` is empty. Cannot proceed.");
            return;
        }
    }
    if (strcmp(command, "register_rfid") == 0)
    {
        if (name == nullptr)
        {
            char message[20];
            snprintf(message, sizeof(message), "ERROR : %d", FAILED_TO_REGISTER_NFC_NO_NAME);
            pCharacteristic->setValue(message);
            pCharacteristic->notify();

            ESP_LOGE(DOOR_INFO_SERVICE_LOG_TAG, "Received 'register_rfid' command, but 'name' is empty. Cannot proceed.");
            return;
        }
    }
    if (strcmp(command, "delete_rfid") == 0)
    {

        if (visitor_id == nullptr)
        {
            char message[20];
            snprintf(message, sizeof(message), "ERROR : %d", FAILED_TO_DELETE_NFC_NO_ID);
            pCharacteristic->setValue(message);
            pCharacteristic->notify();

            ESP_LOGW(DOOR_INFO_SERVICE_LOG_TAG, "Received 'delete_rfid' command but `visitor id` is empty. Cannot proceed.");
            return;
        }
    }

    commandBleData.setCommand(command);
    commandBleData.setName(name);
    commandBleData.setKeyAccess(key_access);
    commandBleData.setVisitorId(visitor_id);
    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Received Valid Data Door Characteristic: Payload = %s", value.c_str());
}

/**
 * @brief BLE callback class for handling write operations on the AC Remote characteristic.
 * 
 */
void ACCharacteristicCallback::onWrite(BLECharacteristic* pCharacteristic){
    const std::string& value = pCharacteristic -> getValue();
    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Incoming AC Characteristic UUID value of %s", value.c_str()); 
    pCharacteristic -> setValue(value);
    
    // Maximum for Notify is 20 Bytes Packages Length 
    pCharacteristic -> notify(); 
}

/**
 * @brief BLE callback class for handling write operations on the LED Remote characteristic.
 * 
 */
void LEDRemoteCharacteristicCallback::onWrite(BLECharacteristic* pCharacteristic){
    const std::string& value = pCharacteristic -> getValue();
    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Incoming LED Remote Characteristic UUID value of %s", value.c_str());
    pCharacteristic->setValue(value);

    // Maximum for Notify is 20 Bytes Packages Length
    pCharacteristic->notify();
}

/**
 * @class DoorInfoService
 * @brief BLE service for managing door, AC remote, LED remote, and notification characteristics.
 */
DoorInfoService::DoorInfoService(BLEServer* pServer) {
    this -> _pServer = pServer;
    this -> _pService = nullptr;
    this -> _pDoorChar = nullptr;
    this -> _pACChar = nullptr;
    this -> _pLedChar = nullptr;
    this -> _pNotificationChar = nullptr;
}

DoorInfoService::~DoorInfoService() {}

 /**
 * @brief Starts the DoorInfoService by creating characteristics and adding them to the service.
 * 
 * This creates characteristics for door, AC remote, LED remote, and notification and starts the service.
 */
void DoorInfoService::startService() {
    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Initializing BLE Door Info Service");
    _pService = _pServer->createService(SERVICE_UUID);

    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Initializing BLE Door Characteristic");
    _pDoorChar = _pService->createCharacteristic(
        DOOR_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_READ  |
        BLECharacteristic::PROPERTY_NOTIFY 
    );
    _pDoorChar -> addDescriptor(new BLE2902());
    _pDoorChar -> setCallbacks(new DoorCharacteristicCallbacks());
  
    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Initializing BLE AC Remote Characteristic");
    _pACChar = _pService->createCharacteristic(
        AC_REMOTE_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_READ 
    );
    _pACChar -> addDescriptor(new BLE2902());
    _pACChar -> setCallbacks(new ACCharacteristicCallback());

    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Initializing BLE LED Remote Characteristic");
    _pLedChar = _pService->createCharacteristic(
        LED_REMOTE_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_READ 
    );
    _pLedChar -> addDescriptor(new BLE2902());
    _pLedChar -> setCallbacks(new LEDRemoteCharacteristicCallback());

    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Initializing BLE Notification Characteristic");
    _pNotificationChar = _pService->createCharacteristic(
        NOTIFICATION_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_NOTIFY |
            BLECharacteristic::PROPERTY_READ);
    _pNotificationChar->addDescriptor(new BLE2902());

    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Add Primary Service for Door Service");
    _pService->start();
}

 /**
 * @brief Sends a JSON notification to the connected client.
 * 
 * This function serializes the JSON document and sends it to the notification characteristic.
 * 
 * @param json The JSON document containing the notification data.
 */
void DoorInfoService::sendNotification(JsonDocument& json){
    String buffer;
    serializeJson(json, buffer);
    _pNotificationChar -> setValue(buffer.c_str());
    _pNotificationChar -> notify();
    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Notification sent to Notification Characteristic!");
}

/**
 * @brief Sends a status and message as a notification to the client.
 * 
 * This function creates a simple JSON document with the status and message and sends it to the notification characteristic.
 * 
 * @param status The status of the notification.
 * @param message The message to send as a notification.
 */
void DoorInfoService::sendNotification(char* status, char* message){
    JsonDocument document;
    document["status"] = status;
    document["message"] = message;

    String buffer;
    serializeJson(document, buffer);
    _pNotificationChar -> setValue(buffer.c_str());
    _pNotificationChar -> notify();
    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Notification sent to Notification Characteristic!");
}