#define LOG_TAG "FINGERPRINT_SERVICE"
#include "FingerprintService.h"
#include <esp_log.h>

FingerprintService::FingerprintService(FingerprintSensor *fingerprintSensor, SDCardModule *sdCardModule, DoorRelay *doorRelay) 
    : _fingerprintSensor(fingerprintSensor), _sdCardModule(sdCardModule), _doorRelay(doorRelay){
    setup();
}

bool FingerprintService::setup(){
    ESP_LOGI(LOG_TAG, "Fingerprint Setup");
    bool result =  _fingerprintSensor -> setup();
    return result;
}

bool FingerprintService::addFingerprint(char* username, int fingerprintId){
    bool addFingerprintResultSensor = _fingerprintSensor -> addFingerprintModel(fingerprintId);
    if (addFingerprintResultSensor){
        bool saveFingerprintSDCard = _sdCardModule -> saveFingerprintToSDCard(username, fingerprintId); 
        return saveFingerprintSDCard;
    }else{
        return true;
    }
}

bool FingerprintService::deleteFingerprint(char* username, int fingerprintId){
    bool deleteFingerprintResultSensor = _fingerprintSensor -> deleteFingerprintModel(fingerprintId);
    if(deleteFingerprintResultSensor){
        bool deleteFingerprintSDCard = _sdCardModule ->deleteFingerprintFromSDCard(username, fingerprintId);
        return deleteFingerprintSDCard;
    }else{
        return false;
    }
}

bool FingerprintService::authenticateAccessFingerprint(){
    int isRegsiteredModel = _fingerprintSensor-> getFingerprintIdModel();
    if(isRegsiteredModel > 0){
        if(_sdCardModule->isFingerprintIdRegistered(isRegsiteredModel)){
            ESP_LOGI(LOG_TAG, "Fingerprint Match with ID %d", isRegsiteredModel);
            
            _fingerprintSensor->toggleSuccessFingerprintLED();
            _doorRelay->toogleRelay();
            return true;
        }
        ESP_LOGI(LOG_TAG, "Fingerprint Model ID %d is Registered on Sensor, but not appear in stored data. Cannot open the Door Lock", isRegsiteredModel);
        
        _fingerprintSensor->toggleFailedFingerprintLED();
        return false;
    }else{
        ESP_LOGW(LOG_TAG, "Fingerprint does not match the stored data. Access denied");
        
        _fingerprintSensor->toggleFailedFingerprintLED();
        return false;
    }
}