#include "FingerprintService.h"
#include <esp_log.h>
#define LOG_TAG "FINGERPRINT_SERVICE"

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