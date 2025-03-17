#define NFC_SERVICE_LOG_TAG "NFC_SERVICE"
#include "NFCService.h"
#include <esp_log.h>

NFCService::NFCService(AdafruitNFCSensor *nfcSensor, SDCardModule *sdCardModule, DoorRelay *doorRelay, BLEModule* bleModule) 
    : _nfcSensor(nfcSensor), _sdCardModule(sdCardModule), _doorRelay(doorRelay), _bleModule(bleModule){
    setup();
}

bool NFCService::setup(){
    ESP_LOGI(NFC_SERVICE_LOG_TAG, "NFC Service Creation");
    return true;
}

/**
 * @brief Add a new NFC card control and save its UID to the SD card.
 * 
 * This function attempts to read the NFC card's UID and saves it to the SD card under the 
 * specified username.
 * 
 * @param username The username for NFC registration.
 * @return true if NFC UID is successfully saved to the SD card, false otherwise.
 */
bool NFCService::addNFC(const char* username) {
    ESP_LOGI(NFC_SERVICE_LOG_TAG, "Enrolling new NFC Access User! Username: %s", username);

    uint16_t timeout = 10000;
    ESP_LOGI(NFC_SERVICE_LOG_TAG, "Awaiting NFC Card Input! Timeout %d ms", timeout);
    char* uidCard = _nfcSensor -> readNFCCard(timeout);
    
    if (uidCard == nullptr || uidCard[0] == '\0') {
        ESP_LOGW(NFC_SERVICE_LOG_TAG, "No NFC card detected for User: %s!", username);
        return false;
    } else {
        ESP_LOGI(NFC_SERVICE_LOG_TAG, "NFC card detected for User: %s with UID: %s", username, uidCard);
        bool saveNFCtoSDCard = _sdCardModule->saveNFCToSDCard(username, uidCard);
        
        if (saveNFCtoSDCard) {
            // Prepare the data payload
            sendbleNotification("OK", username,  uidCard, "NFC Card registered successfully!");
            ESP_LOGI(NFC_SERVICE_LOG_TAG, "NFC UID %s successfully saved to SD card for User: %s", uidCard, username);
        } else {
            // Prepare the data payload
            sendbleNotification("ERR", username,  uidCard, "Failed to register NFC card!");
            ESP_LOGE(NFC_SERVICE_LOG_TAG, "Failed to save NFC UID %s to SD card for User: %s", uidCard, username);
        }
        return saveNFCtoSDCard; 
    }
}

/**
 * @brief Delete an NFC card from the SD card associated with the given username.
 * 
 * This function attempts to delete the NFC card UID associated with a user from the SD card.
 * If the NFC card UID is empty or passed as "null", no deletion will occur.
 * 
 * @param username The username whose NFC card is to be deleted.
 * @param uidCard The UID of the NFC card to be deleted.
 * @return true if the NFC UID was successfully deleted from the SD card, false otherwise.
 */
bool NFCService::deleteNFC(const char* username, const char* uidCard) {
    ESP_LOGI(NFC_SERVICE_LOG_TAG, "Deleting NFC User! Username = %s, NFC UID = %s", username, uidCard);

    // Check if the UID card is empty or if it's "null"
    if (uidCard == nullptr || uidCard[0] == '\0' || strcmp(uidCard, "null") == 0) {
        ESP_LOGW(NFC_SERVICE_LOG_TAG, "There's no card detected or ID Card was passed as 'null'. Proceeding with not deleting anything.");
        return false; 
    } else {
        bool deleteNFCfromSDCard = _sdCardModule->deleteNFCFromSDCard(username, uidCard);

        if (deleteNFCfromSDCard) {
            // Prepare the data payload
            sendbleNotification("OK", username,  uidCard, "NFC Card deleted successfully!");
            ESP_LOGI(NFC_SERVICE_LOG_TAG, "NFC card deleted successfully for User: %s, NFC UID: %s", username, uidCard);
        } else {
            // Prepare the data payload
            sendbleNotification("ERR", username,  uidCard, "Failed to delete NFC card!");
            ESP_LOGE(NFC_SERVICE_LOG_TAG, "Failed to delete NFC card for User: %s, NFC UID: %s", username, uidCard);
        }
        return deleteNFCfromSDCard;
    }
}

/**
 * @brief Authenticate access using an NFC card.
 * 
 * This function reads the NFC card and checks if the UID is registered in the system.
 * 
 * @return true if the NFC card UID matches a registered entry and access is granted; 
 *         false if no card is detected or the UID is not registered.
 */
bool NFCService::authenticateAccessNFC(){
    char* uidCard = _nfcSensor -> readNFCCard();
    if (uidCard == nullptr || uidCard[0] == '\0') { return false; } 
    else {
        if(_sdCardModule->isNFCIdRegistered(uidCard)){
            ESP_LOGI(NFC_SERVICE_LOG_TAG, "NFC Card Match with ID %s", uidCard);
            _doorRelay->toggleRelay();
            return true;
        }
        // TODO : Perhaps implement callback that tell the FingerprintModel is save correctly, but the data is not save into the Microcontroller System
        ESP_LOGI(NFC_SERVICE_LOG_TAG, "NFC Card ID %s is detected but not stored in our data system!", uidCard);
        return false;
    }
}

void NFCService::sendbleNotification(char* status, const char* username, const char* uidCard, char* message){
    JsonDocument doc;
    doc["data"]["name"] = username;
    doc["data"]["key_access"] = uidCard;
    _bleModule -> sendReport(status, doc.as<JsonObject>(), message);
}
