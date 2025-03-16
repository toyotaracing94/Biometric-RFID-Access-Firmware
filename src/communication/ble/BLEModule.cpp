#define BLE_MODULE_LOG_TAG "BLE"
#include <esp_log.h>
#include "BLEModule.h"

#include "nvs_flash.h"

#include "communication/ble/service/DeviceInfoService.h"

BLEModule::BLEModule(){}

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
    _bleServer -> setCallbacks(new BLEServerCallbacks());
    _bleService = _bleServer -> createService(SERVICE_UUID);
}

void BLEModule::setupCharacteristic(){
    ESP_LOGI(BLE_MODULE_LOG_TAG, "Initializing BLE Characteristic...");

    DeviceInfoService deviceInfoService(_bleServer);
    deviceInfoService.startService();

    // Start Overall Service
    _bleService -> start();  

    // Broadcast the BLE Connection to external
    _bleAdvertise = BLEDevice::getAdvertising();  
    _bleAdvertise -> addServiceUUID(SERVICE_UUID);  
    _bleAdvertise -> setScanResponse(true);  
    _bleAdvertise -> setMinPreferred(0x06);  
    _bleAdvertise -> setMinPreferred(0x12);  
    BLEDevice::startAdvertising();  
    
    ESP_LOGI(BLE_MODULE_LOG_TAG, "BLE initialized. Waiting for client to connect...");  
}