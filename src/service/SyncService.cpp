#define SYNC_SERVICE_LOG_TAG "SYNC_SERVICE"
#include "SyncService.h"

SyncService::SyncService(SDCardModule *sdCardModule, BLEModule* bleModule) 
    : _sdCardModule(sdCardModule), _bleModule(bleModule){}


void SyncService::sync(){
    ESP_LOGI(SYNC_SERVICE_LOG_TAG, "Start Sync to Titan by Sending Data in ESP32");
    JsonDocument object = _sdCardModule -> syncData();
    JsonObject payload = object.as<JsonObject>();
    
    char status[3] = "OK";
    char message[50] = "Success getting list of key Access!";
    _bleModule -> sendReport(status, payload, message);
}