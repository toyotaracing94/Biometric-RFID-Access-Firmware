#include "FingerprintService.h"
#include <esp_log.h>
#define LOG_TAG "FINGERPRINT_SERVICE"

FingerprintService::FingerprintService(FingerprintManager *manager) : _fingerprintManager(manager) {}

bool FingerprintService::setup(){
    ESP_LOGI(LOG_TAG, "Fingerprint Setup");
    bool result =  _fingerprintManager -> setup();
    return result;
}

bool FingerprintService::addFingerprint(char* username, int fingerprintId){
    // Add the Fingerprint Image to the sensor
    bool addFingerprintResultSensor = _fingerprintManager -> addFingerprintModel(fingerprintId);

    // Add the Fingerprint ID of the relevant Image to SD Card
    return true;
}