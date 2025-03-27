#define BLE_MODULE_LOG_TAG "BLE"
#include <esp_log.h>
#include "BLEModule.h"

BLEModule::BLEModule() {}

/**
 * @brief Initializes the BLE server and services.
 * 
 * This method initializes the BLE device, creates the BLE server, and sets the callbacks for handling
 * BLE events. It is intended to be called when the BLE module is first initialized.
 *
 * @see BLEDevice::init(), BLEDevice::createServer(), BLEServer::setCallbacks()
 */
void BLEModule::initBLE(){
    ESP_LOGI(BLE_MODULE_LOG_TAG, "Initializing BLE Server and Service...");
    
    BLEDevice::init(BLESERVERNAME);
    _bleServer = BLEDevice::createServer();
    _bleServer -> setCallbacks(new BLECallback());
}

/**
 * @brief Sets up BLE characteristics, services, and advertising of each service of the BLE
 * 
 * This method registers any kind of services for the BLE Service Channel, then configures them into BLE advertising
 * to make the device discoverable by clients. It is called after the BLE server is initialized.
 *
 * @see BLEDevice::getAdvertising(), BLEAdvertising::setScanResponse(), BLEDevice::startAdvertising()
 */
void BLEModule::setupCharacteristic(){
    // Register BLE Service
    _deviceInfoService = new DeviceInfoService(_bleServer);
    _deviceInfoService->startService();

    _doorInfoService = new DoorInfoService(_bleServer);
    _doorInfoService->startService();

    _bleAdvertise = BLEDevice::getAdvertising();
    _bleAdvertise -> setScanResponse(true);  
    _bleAdvertise -> setMinPreferred(0x06);  
    _bleAdvertise -> setMinPreferred(0x12);  
    BLEDevice::startAdvertising();  
    
    ESP_LOGI(BLE_MODULE_LOG_TAG, "BLE initialized. Waiting for client to connect...");  
}

/**
 * @brief Sends a notification with the given status, payload, and message.
 * 
 * This method constructs a JSON document containing the status, payload, and message, and sends it as
 * a notification to the door information service.
 *
 * @param status The status to be included in the notification (e.g., "OK", "ERROR").
 * @param payload The JSON object containing data to be sent in the notification.
 * @param message A string message to be included in the notification.
 *
 * @see DoorInfoService::sendNotification()
 */
void BLEModule::sendReport(const char* status, const JsonObject& payload, const char* message){
    ESP_LOGI(BLE_MODULE_LOG_TAG, "Sending notification to door notification characteristic");

    JsonDocument document;
    document["status"]  = status;
    document["data"]    = payload;
    document["message"] = message;
    _doorInfoService -> sendNotification(document);
}