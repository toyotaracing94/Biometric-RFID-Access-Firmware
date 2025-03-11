#define LOG_TAG "NFC_SERVICE"
#include "NFCService.h"
#include <esp_log.h>

NFCService::NFCService(AdafruitNFCSensor *nfcSensor, SDCardModule *sdCardModule, DoorRelay *doorRelay) 
    : _nfcSensor(nfcSensor), _sdCardModule(sdCardModule), _doorRelay(doorRelay){
    setup();
}

bool NFCService::setup(){
    ESP_LOGI(LOG_TAG, "NFC Service Creation");
    return true;
}

bool NFCService::addNFC(char* username) {
    ESP_LOGI(LOG_TAG, "Enrolling new NFC Access User! Username: %s", username);
    char* uidCard = _nfcSensor -> readNFCCard();
    
    if (uidCard == nullptr || uidCard[0] == '\0') {
        ESP_LOGW(LOG_TAG, "No NFC card detected for User: %s!", username);
        return false;
    } else {
        ESP_LOGI(LOG_TAG, "NFC card detected for User: %s with UID: %s", username, uidCard);
        bool saveNFCtoSDCard = _sdCardModule->saveNFCToSDCard(username, uidCard);
        
        if (saveNFCtoSDCard) {
            ESP_LOGI(LOG_TAG, "NFC UID %s successfully saved to SD card for User: %s", uidCard, username);
        } else {
            ESP_LOGE(LOG_TAG, "Failed to save NFC UID %s to SD card for User: %s", uidCard, username);
        }
        return saveNFCtoSDCard; 
    }
}

bool NFCService::deleteNFC(char* username, char* uidCard) {
    ESP_LOGI(LOG_TAG, "Deleting NFC User! Username = %s, NFC UID = %s", username, uidCard);

    // Check if the UID card is empty or if it's "null"
    if (uidCard == nullptr || uidCard[0] == '\0' || strcmp(uidCard, "null") == 0) {
        ESP_LOGW(LOG_TAG, "There's no card detected or ID Card was passed as 'null'. Proceeding with not deleting anything.");
        return false; 
    } else {
        bool deleteNFCfromSDCard = _sdCardModule->deleteNFCFromSDCard(username, uidCard);

        if (deleteNFCfromSDCard) {
            ESP_LOGI(LOG_TAG, "NFC card deleted successfully for User: %s, NFC UID: %s", username, uidCard);
        } else {
            ESP_LOGE(LOG_TAG, "Failed to delete NFC card for User: %s, NFC UID: %s", username, uidCard);
        }
        return deleteNFCfromSDCard;
    }
}

bool NFCService::authenticateAccessNFC(){
    char* uidCard = _nfcSensor -> readNFCCard();
    if (uidCard == nullptr || uidCard[0] == '\0') {
        ESP_LOGW(LOG_TAG, "No NFC card detected. Perhaps timeout possibly");
        return false;
    } else {
        if(_sdCardModule->isNFCIdRegistered(uidCard)){
            ESP_LOGI(LOG_TAG, "NFC Card Match with ID %s", uidCard);
            _doorRelay->toggleRelay();
            return true;
        }
        // TODO : Perhaps implement callback that tell the FingerprintModel is save correctly, but the data is not save into the Microcontroller System
        ESP_LOGI(LOG_TAG, "NFC Card ID %s is detected but not stored in our data system!", uidCard);
        return false;
    }
}
