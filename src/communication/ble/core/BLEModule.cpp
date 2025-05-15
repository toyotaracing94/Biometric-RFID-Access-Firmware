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
 * @see NimBLEDevice::init(), NimBLEDevice::createServer(), NimBLEDevice::setCallbacks()
 */
void BLEModule::initBLE(){
    ESP_LOGI(BLE_MODULE_LOG_TAG, "Initializing BLE Server and Service...");
    
    NimBLEDevice::init(std::string(BLESERVERNAME));
    _bleServer = NimBLEDevice::createServer();
    _bleServer -> setCallbacks(new BLECallback());
}

/**
 * @brief Sets up BLE characteristics and the services
 * 
 * This method registers any kind of custom services for the BLE Service Channel.
 * enabling BLE clients to interact with the services
 *
 * @see DeviceInfoService::startService(), DoorInfoService::startService()
 */
void BLEModule::setupCharacteristic(){
    // Register BLE Service
    _deviceInfoService = new DeviceInfoService(_bleServer);
    _deviceInfoService->startService();

    _doorInfoService = new DoorInfoService(_bleServer);
    _doorInfoService->startService();

    ESP_LOGI(BLE_MODULE_LOG_TAG, "BLE services set up.");
}

/**
 * @brief Configures and starts BLE advertising with device name and registered service UUID.
 * 
 * This function sets up BLE advertising by configuring both the primary advertising data
 * and the scan response data. The device name is included in the advertising payload,
 * while the 128-bit service UUID is included in the scan response. This allows BLE clients
 * to discover the device and retrieve additional information upon scanning.
 * 
 * @details
 * - Advertising Data (`advData`) includes the device name.
 * - Scan Response Data (`scanRespData`) includes the full 128-bit Service UUID.
 * - Advertising is started after configuration is complete.
 */
void BLEModule::setupAdvertising(){
    // Advertising data
    // This is the main data sent during the initial advertisement
    NimBLEAdvertisementData advertisingData;
    advertisingData.setName(BLESERVERNAME);
    advertisingData.addTxPower();
    advertisingData.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);

    // Optional: Add manufacturer-specific data. I'm gonna borrow Toyota Motor Corporation
    // Big-Endian format
    std::string mfgData = std::string("\x77\x09\x01\x02\x03\x04", 6);
    advertisingData.setManufacturerData(mfgData);

    // Scan response data
    // This is data that can be sent as a response to a scan request after the device is discovered
    NimBLEAdvertisementData scanRespData;

    // Add service UUIDs to scan response data
    scanRespData.addServiceUUID(SERVICE_UUID);

    // Set advertising data (main advertisement data + scan response data)
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->setAdvertisementData(advertisingData);
    pAdvertising->setScanResponseData(scanRespData);

    // Start advertising
    pAdvertising->start();
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
[[deprecated("This function is will soon deprecated and remove. Use the new 'sendReport' with int parameter instead.")]]
void BLEModule::sendReport(const char* status, const JsonObject& payload, const char* message){
    ESP_LOGI(BLE_MODULE_LOG_TAG, "Sending notification to door notification characteristic");

    JsonDocument document;
    document["status"]  = status;
    document["data"]    = payload;
    document["message"] = message;
    _doorInfoService -> sendNotification(document);
}

/**
 * @brief Sends a notification with the given status code
 * 
 * This method constructs a JSON document containing the status code only and sends it as
 * a notification to the door information service.
 *
 * @param statusCode the Status Code int
 *
 */
void BLEModule::sendReport(int statusCode){
    ESP_LOGI(BLE_MODULE_LOG_TAG, "Sending notification to door notification characteristic");

    JsonDocument document;
    document["status"]  = statusCode;
    _doorInfoService -> sendNotification(document);
}