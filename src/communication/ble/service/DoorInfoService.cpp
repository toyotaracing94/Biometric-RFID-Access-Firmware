#include "DoorInfoService.h"
#include <stdio.h>
#include "entity/CommandBleData.h"
#include "StatusCodes.h"


DoorCharacteristicCallbacks::DoorCharacteristicCallbacks(NimBLECharacteristic *pNotificationChar) 
    : _pNotificationChar(pNotificationChar){}

/**
 * @brief Handles write operations to the Door characteristic.
 * 
 * @param pCharacteristic Pointer to the characteristic that was written to.
 * @param connInfo        Connection information of the client that wrote the data.
 */
void DoorCharacteristicCallbacks::onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo){
    const std::string& value = pCharacteristic -> getValue();
    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Incoming Door Characteristic UUID value of %s", value.c_str());

    JsonDocument incomingData;
    DeserializationError error = deserializeJson(incomingData, value);

    if (error){
        ESP_LOGE(DOOR_INFO_SERVICE_LOG_TAG, "Failed to deserialize JSON. Error: %s", error.c_str());
        incomingData.clear();

        BLEMessageSender::sendNotification(_pNotificationChar, INVALID_JSON_BLE_REQUEST_FORMAT);
        return;
    }

    JsonObject data = incomingData["data"].as<JsonObject>();
    const char *command = incomingData["command"];
    const char *name = data["name"];
    const char *key_access = nullptr;
    const char *visitor_id = nullptr;

    if (command != nullptr)
        ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Received Name: Command = %s", command);

    if (!data.isNull()){
        if (name != nullptr)
            ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Received Name: Name = %s", name);
        if (data["key_access_id"].is<const char *>())
        {
            key_access = data["key_access_id"].as<const char *>();
            ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Received Data: Key Access = %s", key_access);
        }
        else if (data["key_access_id"].is<int>())
        {
            int key_access_int = data["key_access_id"].as<int>();
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
    if (strcmp(command, "register_fp") == 0){
        if (name == nullptr && visitor_id != nullptr && key_access != nullptr){
            ESP_LOGE(DOOR_INFO_SERVICE_LOG_TAG, "Received 'register_fp' command, but 'name' is empty. Cannot proceed.");
            BLEMessageSender::sendNotification(_pNotificationChar, FAILED_TO_REGISTER_FINGERPRINT_NO_NAME);
            return;
        }
        
        if (visitor_id == nullptr || key_access == nullptr){
            ESP_LOGE(DOOR_INFO_SERVICE_LOG_TAG, "Received 'register_fp' command, but `visitor_id` or `key_access` is empty. Cannot proceed.");
            BLEMessageSender::sendNotification(_pNotificationChar, FAILED_TO_DELETE_FINGERPRINT_NO_ID);
            return;
        }

        if (name == nullptr && visitor_id == nullptr && key_access == nullptr){
            ESP_LOGE(DOOR_INFO_SERVICE_LOG_TAG, "Received 'register_fp' command, but 'name', `visitor_id`, `key_access` is empty. Cannot proceed.");
            BLEMessageSender::sendNotification(_pNotificationChar, FAILED_TO_DELETE_FINGERPRINT_NO_NAME_AND_ID);
            return;
        }
    }

    if (strcmp(command, "delete_fp") == 0){
        if (key_access == nullptr){
            ESP_LOGW(DOOR_INFO_SERVICE_LOG_TAG, "Received 'delete_fp' command but `visitor_id` is empty. Cannot proceed.");
            BLEMessageSender::sendNotification(_pNotificationChar, FAILED_TO_DELETE_FINGERPRINT_NO_ID);
            return;
        }
    }

    if (strcmp(command, "delete_fp_user") == 0){
        if (visitor_id == nullptr){
            ESP_LOGW(DOOR_INFO_SERVICE_LOG_TAG, "Received 'delete_fp_user' command but `visitor_id` is empty. Cannot proceed.");
            BLEMessageSender::sendNotification(_pNotificationChar, FAILED_TO_DELETE_FINGERPRINT_NO_ID);
            return;
        }
    }

    if (strcmp(command, "register_rfid") == 0){
        if (name == nullptr && visitor_id != nullptr && key_access != nullptr){
            ESP_LOGE(DOOR_INFO_SERVICE_LOG_TAG, "Received 'register_rfid' command, but 'name' is empty. Cannot proceed.");
            BLEMessageSender::sendNotification(_pNotificationChar, FAILED_TO_REGISTER_NFC_NO_NAME);
            return;
        }
        
        if (visitor_id == nullptr || key_access == nullptr){
            ESP_LOGE(DOOR_INFO_SERVICE_LOG_TAG, "Received 'register_rfid' command, but `visitor_id` or `key_access` is empty. Cannot proceed.");
            BLEMessageSender::sendNotification(_pNotificationChar, FAILED_TO_REGISTER_NFC_NO_VISITOR_ID_OR_KEY_ACCESS_ID);
            return;
        }

        if (name == nullptr && visitor_id == nullptr && key_access == nullptr){
            ESP_LOGE(DOOR_INFO_SERVICE_LOG_TAG, "Received 'register_rfid' command, but 'name', `visitor_id`, `key_access` is empty. Cannot proceed.");
            BLEMessageSender::sendNotification(_pNotificationChar, FAILED_TO_REGISTER_NFC_NO_NAME_AND_ID);
            return;
        }
    }
    
    if (strcmp(command, "delete_rfid") == 0){
        if (key_access == nullptr){
            ESP_LOGW(DOOR_INFO_SERVICE_LOG_TAG, "Received 'delete_rfid' command but `key access id` is empty. Cannot proceed.");
            BLEMessageSender::sendNotification(_pNotificationChar, FAILED_TO_DELETE_NFC_NO_ID);
            return;
        }
    }

    if (strcmp(command, "delete_rfid_user") == 0){
        if (visitor_id == nullptr){
            ESP_LOGW(DOOR_INFO_SERVICE_LOG_TAG, "Received 'delete_rfid_user' command but `visitor_id` is empty. Cannot proceed.");
            BLEMessageSender::sendNotification(_pNotificationChar, FAILED_TO_DELETE_NFC_NO_ID);
            return;
        }
    }

    if (strcmp(command, "delete_access_user") == 0){
        if (visitor_id == nullptr){
            ESP_LOGW(DOOR_INFO_SERVICE_LOG_TAG, "Received 'delete_access_user' command but `visitor_id` is empty. Cannot proceed.");
            BLEMessageSender::sendNotification(_pNotificationChar, FAILED_TO_DELETE_NFC_NO_ID);
            return;
        }
    }
    
    commandBleData.setCommand(command);
    commandBleData.setName(name);
    commandBleData.setKeyAccess(key_access);
    commandBleData.setVisitorId(visitor_id);

    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Received Valid Data Door Characteristic: Payload = %s", value.c_str());
    vTaskDelay( 50 / portTICK_PERIOD_MS);
}

/**
 * @brief BLE callback class for handling write operations on the AC Remote characteristic.
 * 
 * @param pCharacteristic Pointer to the characteristic that was written to.
 * @param connInfo        Connection information of the client that wrote the data.
 */
void ACCharacteristicCallback::onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo){
    const std::string& value = pCharacteristic -> getValue();
    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Incoming AC Characteristic UUID value of %s", value.c_str()); 
    pCharacteristic -> setValue(value);
    
    // Maximum for Notify is 20 Bytes Packages Length 
    pCharacteristic -> notify();
    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Notified AC Remote characteristic with updated value (Length: %d bytes)", value.length());
}

/**
 * @brief BLE callback class for handling write operations on the LED Remote characteristic.
 * 
 * @param pCharacteristic Pointer to the characteristic that was written to.
 * @param connInfo        Connection information of the client that wrote the data.
 */
void LEDRemoteCharacteristicCallback::onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo){
    const std::string& value = pCharacteristic -> getValue();
    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Incoming LED Remote Characteristic UUID value of %s", value.c_str());
    pCharacteristic->setValue(value);

    // Maximum for Notify is 20 Bytes Packages Length
    pCharacteristic->notify();
    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Notified LED Remote characteristic with updated value (Length: %d bytes)", value.length());
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
 * 
 * @note
 * See this for more reference
 * 
 * 1. For descriptor --> https://h2zero.github.io/NimBLE-Arduino/md__migration__guide.html#descriptors
 * 2. For chacateristic descriptor again? --> https://github.com/h2zero/esp-nimble-cpp/issues/2
 */
void DoorInfoService::startService() {
    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Initializing BLE Door Info Service");
    _pService = _pServer->createService(SERVICE_UUID);

    // Initialize BLE Notification Characteristic with Notify property
    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Initializing BLE Notification Characteristic");
    _pNotificationChar = _pService->createCharacteristic(
        NOTIFICATION_CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ
    );
    NimBLEDescriptor* notifDesc = new NimBLEDescriptor(
        NimBLEUUID((uint16_t)0x2901), NIMBLE_PROPERTY::READ, 64, _pDoorChar); 
    notifDesc->setValue("Notification Characteristic");
    _pNotificationChar->addDescriptor(notifDesc);

    // Initialize BLE Door Characteristic with Notify property
    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Initializing BLE Door Characteristic");
    _pDoorChar = _pService->createCharacteristic(
        DOOR_CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY
    );
    _pDoorChar->setCallbacks(new DoorCharacteristicCallbacks(_pNotificationChar));
    NimBLEDescriptor* doorDesc = new NimBLEDescriptor(
        NimBLEUUID((uint16_t)0x2901), NIMBLE_PROPERTY::READ, 64, _pDoorChar); 
    doorDesc->setValue("Door Characteristic");
    _pDoorChar->addDescriptor(doorDesc);

    // Initialize BLE AC Remote Characteristic
    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Initializing BLE AC Remote Characteristic");
    _pACChar = _pService->createCharacteristic(
        AC_REMOTE_CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY
    );
    _pACChar->setCallbacks(new ACCharacteristicCallback());
    NimBLEDescriptor* acRemoteDesc = new NimBLEDescriptor(
        NimBLEUUID((uint16_t)0x2901), NIMBLE_PROPERTY::READ, 64, _pDoorChar); 
    acRemoteDesc->setValue("AC Remote Characteristic");
    _pACChar->addDescriptor(acRemoteDesc);

    // Initialize BLE LED Remote Characteristic
    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Initializing BLE LED Remote Characteristic");
    _pLedChar = _pService->createCharacteristic(
        LED_REMOTE_CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY
    );
    _pLedChar->setCallbacks(new LEDRemoteCharacteristicCallback());
    NimBLEDescriptor* ledDesc = new NimBLEDescriptor(
        NimBLEUUID((uint16_t)0x2901), NIMBLE_PROPERTY::READ, 64, _pDoorChar); 
    ledDesc->setValue("LED Remote Characteristic");
    _pLedChar->addDescriptor(ledDesc);

    // Start the service
    ESP_LOGI(DOOR_INFO_SERVICE_LOG_TAG, "Starting Door Info Service");
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
 * @brief Sends a status as a notification to the client.
 * 
 * This function creates a simple JSON document with the status sends it to the notification characteristic.
 * 
 * @param status The status of the notification.
 */
void DoorInfoService::sendNotification(char* status){ 
    JsonDocument document;
    document["status"] = status;

    String buffer;
    serializeJson(document, buffer);
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