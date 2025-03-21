#define FINGERPRINT_SERVICE_LOG_TAG "FINGERPRINT_SERVICE"
#include "FingerprintService.h"
#include <esp_log.h>

FingerprintService::FingerprintService(FingerprintSensor *fingerprintSensor, SDCardModule *sdCardModule, DoorRelay *doorRelay, BLEModule *bleModule) 
    : _fingerprintSensor(fingerprintSensor), _sdCardModule(sdCardModule), _doorRelay(doorRelay), _bleModule(bleModule){
    setup();
}

bool FingerprintService::setup(){
    ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Fingerprint Service Creation");
    return true;
}

/**
 * @brief Add a new fingerprint access control
 * 
 * This function will attempts to add a new fingerprint model to the fingerprint sensor using 
 * the given fingerprint ID and add the associate ID of that model to save it to SD Card
 * 
 * @param username The username to associate with the fingerprint ID.
 * @param fingerprintId The unique ID of the fingerprint being enrolled to match with Fingerprint Model on the Sensor.
 * @return 
 *      - true if the fingerprint model was successfully added and Fingerprint ID saved to the SD card; false otherwise.
 */
bool FingerprintService::addFingerprint(const char* username){
    uint8_t fingerprintId = generateFingerprintId();
    ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Enroling new Fingerprint User! Username %s FingerprintID %d", username, fingerprintId);
    bool addFingerprintResultSensor = _fingerprintSensor -> addFingerprintModel(fingerprintId);

    if (addFingerprintResultSensor){
        ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Fingerprint model added successfully for FingerprintID: %d. Saving to SD card...", fingerprintId);
        bool saveFingerprintSDCard = _sdCardModule -> saveFingerprintToSDCard(username, fingerprintId); 
        
        if (saveFingerprintSDCard) {
            // Prepare the data payload
            char status[5] = "OK";
            char message[50] = "Fingerprint registered successfully!";
            sendbleNotification(status, username,  fingerprintId, message);
            ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Fingerprint saved to SD card successfully for User: %s, FingerprintID: %d", username, fingerprintId);
        } else {
            // Prepare the data payload
            char status[5] = "ERR";
            char message[50] = "Failed to register Fingerprint!";
            sendbleNotification(status, username,  fingerprintId, message);
            ESP_LOGE(FINGERPRINT_SERVICE_LOG_TAG, "Failed to save fingerprint to SD card for User: %s, FingerprintID: %d", username, fingerprintId);
        }
        return saveFingerprintSDCard;
    }else{
        ESP_LOGE(FINGERPRINT_SERVICE_LOG_TAG, "Failed to add fingerprint model for FingerprintID: %d", fingerprintId);
        return addFingerprintResultSensor;
    }
}

/**
 * @brief Delete a fingerprint access control
 * 
 * This function attempts to delete a fingerprint model from the fingerprint sensor and 
 * also removes the associated fingerprint data from the SD card.
 * 
 * @param username The username associated with the fingerprint ID to be deleted.
 * @param fingerprintId The unique ID of the fingerprint to be deleted from the sensor and SD card.
 * @return true if the fingerprint was successfully deleted from both the sensor and the SD card;
 *         false otherwise.
 */
bool FingerprintService::deleteFingerprint(const char* username, int fingerprintId){
    ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Deleting Fingerprint User! Username = %s, FingerprintID = %d", username, fingerprintId);
    bool deleteFingerprintResultSensor = _fingerprintSensor -> deleteFingerprintModel(fingerprintId);

    if(deleteFingerprintResultSensor){
        ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Fingerprint model deleted successfully for FingerprintID: %d", fingerprintId);
        bool deleteFingerprintSDCard = _sdCardModule ->deleteFingerprintFromSDCard(username, fingerprintId);

        if (deleteFingerprintSDCard) {
            // Prepare the data payload
            char status[5] = "OK";
            char message[50] = "Fingerprint deleted successfully!";
            sendbleNotification(status, username,  fingerprintId, message);
            ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Fingerprint deleted from SD card successfully for User: %s, FingerprintID: %d", username, fingerprintId);
        } else {
            // Prepare the data payload
            char status[5] = "ERR";
            char message[50] = "Failed to delete Fingerprint!";
            sendbleNotification(status, username,  fingerprintId, message);
            ESP_LOGE(FINGERPRINT_SERVICE_LOG_TAG, "Failed to delete fingerprint from SD card for User: %s, FingerprintID: %d", username, fingerprintId);
        }
        return deleteFingerprintSDCard;
    }else{
        ESP_LOGE(FINGERPRINT_SERVICE_LOG_TAG, "Failed to delete fingerprint model for FingerprintID: %d", fingerprintId);
        return false;
    }
}

/**
 * @brief Authenticate a fingerprint and grant access if registered.
 * 
 * This function checks if the fingerprint captured by the sensor matches a registered fingerprint.
 * If the fingerprint is recognized and registered in the SD card, it will trigger the door relay 
 * to grant access. If the fingerprint is recognized by the sensor but not registered in the system,
 * the access will be denied. If the fingerprint doesn't match, access will also be denied.
 * 
 * @return true if the fingerprint is successfully authenticated and access is granted;
 *         false if the fingerprint is not recognized or registered.
 */
bool FingerprintService::authenticateAccessFingerprint(){
    int isRegsiteredModel = _fingerprintSensor-> getFingerprintIdModel();
    if(isRegsiteredModel > 0){
        if(_sdCardModule->isFingerprintIdRegistered(isRegsiteredModel)){
            ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Fingerprint Match with ID %d", isRegsiteredModel);
            _doorRelay->toggleRelay();
            return true;
        }
        // TODO : Perhaps implement callback that tell the FingerprintModel is save correctly, but the data is not save into the Microcontroller System
        ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Fingerprint Model ID %d is Registered on Sensor, but not appear in stored data. Cannot open the Door Lock", isRegsiteredModel);
        return false;
    }else{
        ESP_LOGD(FINGERPRINT_SERVICE_LOG_TAG, "Fingerprint does not match the stored data. Access denied");
        return false;
    }
}

uint8_t FingerprintService::generateFingerprintId(){
    uint8_t fingerprintId;
    while (true) {
      fingerprintId = random(1,20);
      // Check if the generated fingerprint ID exists on the SD card
      if (!_sdCardModule->isFingerprintIdRegistered(fingerprintId)) {
        // If the ID doesn't exist on the SD card, it's valid
        ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Generated valid fingerprint ID: %d", fingerprintId);
        break;
      } else {
        // If the ID exists on the SD card, try again
        ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Fingerprint ID %d already exists, generating a new ID...", fingerprintId);
      }
    }
    return fingerprintId;
}

void FingerprintService::sendbleNotification(char* status, const char* username, int fingerprintId, char* message){
    JsonDocument doc;
    doc["data"]["name"] = username;
    doc["data"]["key_access"] = fingerprintId;
    _bleModule -> sendReport(status, doc.as<JsonObject>(), message);
}