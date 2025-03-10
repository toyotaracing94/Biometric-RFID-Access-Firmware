#define LOG_TAG "FINGERPRINT_SERVICE"
#include "FingerprintService.h"
#include <esp_log.h>

FingerprintService::FingerprintService(FingerprintSensor *fingerprintSensor, SDCardModule *sdCardModule, DoorRelay *doorRelay) 
    : _fingerprintSensor(fingerprintSensor), _sdCardModule(sdCardModule), _doorRelay(doorRelay){
    setup();
}

bool FingerprintService::setup(){
    ESP_LOGI(LOG_TAG, "Start Fingerprint Setup");
    bool result =  _fingerprintSensor -> setup();
    return result;
}

bool FingerprintService::addFingerprint(char* username, int fingerprintId){
    ESP_LOGI(LOG_TAG, "Enroling new Fingerprint User! Username %s FingerprintID %d", username, fingerprintId);
    bool addFingerprintResultSensor = _fingerprintSensor -> addFingerprintModel(fingerprintId);

    if (addFingerprintResultSensor){
        ESP_LOGI(LOG_TAG, "Fingerprint model added successfully for FingerprintID: %d. Saving to SD card...", fingerprintId);
        bool saveFingerprintSDCard = _sdCardModule -> saveFingerprintToSDCard(username, fingerprintId); 
        
        if (saveFingerprintSDCard) {
            ESP_LOGI(LOG_TAG, "Fingerprint saved to SD card successfully for User: %s, FingerprintID: %d", username, fingerprintId);
        } else {
            ESP_LOGE(LOG_TAG, "Failed to save fingerprint to SD card for User: %s, FingerprintID: %d", username, fingerprintId);
        }
        return saveFingerprintSDCard;
    }else{
        ESP_LOGE(LOG_TAG, "Failed to add fingerprint model for FingerprintID: %d", fingerprintId);
        return addFingerprintResultSensor;
    }
}

bool FingerprintService::deleteFingerprint(char* username, int fingerprintId){
    ESP_LOGI(LOG_TAG, "Deleting Fingerprint User! Username = %s, FingerprintID = %d", username, fingerprintId);
    bool deleteFingerprintResultSensor = _fingerprintSensor -> deleteFingerprintModel(fingerprintId);

    if(deleteFingerprintResultSensor){
        ESP_LOGI(LOG_TAG, "Fingerprint model deleted successfully for FingerprintID: %d", fingerprintId);
        bool deleteFingerprintSDCard = _sdCardModule ->deleteFingerprintFromSDCard(username, fingerprintId);

        if (deleteFingerprintSDCard) {
            ESP_LOGI(LOG_TAG, "Fingerprint deleted from SD card successfully for User: %s, FingerprintID: %d", username, fingerprintId);
        } else {
            ESP_LOGE(LOG_TAG, "Failed to delete fingerprint from SD card for User: %s, FingerprintID: %d", username, fingerprintId);
        }
        return deleteFingerprintSDCard;
    }else{
        ESP_LOGE(LOG_TAG, "Failed to delete fingerprint model for FingerprintID: %d", fingerprintId);
        return false;
    }
}

bool FingerprintService::authenticateAccessFingerprint(){
    int isRegsiteredModel = _fingerprintSensor-> getFingerprintIdModel();
    if(isRegsiteredModel > 0){
        if(_sdCardModule->isFingerprintIdRegistered(isRegsiteredModel)){
            ESP_LOGI(LOG_TAG, "Fingerprint Match with ID %d", isRegsiteredModel);
            _doorRelay->toggleRelay();
            return true;
        }
        // TODO : Perhaps implement callback that tell the FingerprintModel is save correctly, but the data is not save into the Microcontroller System
        ESP_LOGI(LOG_TAG, "Fingerprint Model ID %d is Registered on Sensor, but not appear in stored data. Cannot open the Door Lock", isRegsiteredModel);
        return false;
    }else{
        ESP_LOGW(LOG_TAG, "Fingerprint does not match the stored data. Access denied");
        return false;
    }
}