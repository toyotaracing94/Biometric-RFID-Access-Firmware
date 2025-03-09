#define LOG_TAG "FINGERPRINT_SERVICE"
#include "FingerprintService.h"
#include <esp_log.h>

FingerprintService::FingerprintService(FingerprintSensor *fingerprintSensor, SDCardModule *sdCardModule) 
    : _fingerprintSensor(fingerprintSensor), _sdCardModule(sdCardModule){
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
            // TODO : Relay Control and Fingerprint LED Control
            return true;
        }
        ESP_LOGI(LOG_TAG, "Fingerprint Model ID %d is Registered on Sensor, but not appear in stored data. Cannot open the Door Lock", isRegsiteredModel);
        // TODO: Fingerprint LED Control
        return false;
    }else{
        ESP_LOGW(LOG_TAG, "Fingerprint does not match the stored data. Access denied");
        // TODO: Fingerprint LED Control
        return false;
    }
}