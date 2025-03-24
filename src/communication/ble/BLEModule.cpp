#define BLE_MODULE_LOG_TAG "BLE"
#include <esp_log.h>
#include "BLEModule.h"

#include <nvs_flash.h>

BLEModule::BLEModule() {}

void BLEModule::initBLE(){
    ESP_LOGI(BLE_MODULE_LOG_TAG, "Initializing BLE Server and Service...");
   
    // https://www.esp32.com/viewtopic.php?t=26365
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
    
    BLEDevice::init(BLESERVERNAME);
    _bleServer = BLEDevice::createServer();
    _bleServer -> setCallbacks(new BLECallback());
}

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

void BLEModule::sendReport(const char* status, const JsonObject& payload, const char* message){
    ESP_LOGI(BLE_MODULE_LOG_TAG, "Sending notification to door notification characteristic");

    JsonDocument document;
    document["status"]  = status;
    document["data"]    = payload;
    document["message"] = message;
    _doorInfoService -> sendNotification(document);
}