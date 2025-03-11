#define LOG_TAG "SD_CARD"

#include "esp_log.h"
#include "SDCardModule.h"
#include <ArduinoJson.h>

SDCardModule::SDCardModule() {}

bool SDCardModule::setup(){
    ESP_LOGI(LOG_TAG, "Start SD Card Module Setup!");
    ESP_LOGI(LOG_TAG, "Initializing the SPI! SCK PIN %d, MISO PIN %d, CS_PIN %d", SCK_PIN, MISO_PIN, CS_PIN);
    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
    
    int retries = 1;
    const int maxRetries = 5;
    unsigned long backoffTime = 1000;
    
    while (retries <= maxRetries) {
        if (SD.begin(CS_PIN, SPI)) {
            ESP_LOGI(LOG_TAG, "SD Card has been found and ready!");

            uint8_t cardType = SD.cardType();
            ESP_LOGI(LOG_TAG, "Card type %d", cardType);
            return true; 
        } 
        else {
        ESP_LOGE(LOG_TAG, "SD Card Module not found! Retrying %d", retries);
        retries++;
        delay(backoffTime);
    
        // Exponential backoff: double the wait time after each failure
        backoffTime *= 2;
        }
    }
    
    ESP.restart();
    return false;
}

bool SDCardModule::isFingerprintIdRegistered(int id){
    ESP_LOGI(LOG_TAG, "Checking Fingerprint Model with ID %d in SD Card already exist", id);

    // Open the file and then close it and create new Static in heap
    File file = SD.open(FINGERPRINT_FILE_PATH, FILE_READ);
    JsonDocument document;

    if(file){
        DeserializationError error = deserializeJson(document, file);
        if (error) {
            ESP_LOGE(LOG_TAG, "Failed to deserialize JSON: %s", error.c_str());
            file.close();
            return false;
        }
        file.close();
    }else{
        ESP_LOGE(LOG_TAG, "Error opening the File!, File %s", FINGERPRINT_FILE_PATH);
        return false;
    }

    for (JsonObject user : document.as<JsonArray>()) {
        const char* currentUsername = user["name"];

        JsonArray fingerprintIds = user["key_access"].as<JsonArray>();

        for (int i = 0; i < fingerprintIds.size(); ++i) {
            if (fingerprintIds[i].as<uint8_t>() == id) {
              ESP_LOGI(LOG_TAG, "Fingerprint ID found %d under User %s", id, currentUsername);
              return true;
            }
        }
    }
    ESP_LOGW(LOG_TAG, "Fingerprint ID %d not found in any user", id);
    return false;
}

bool SDCardModule::saveFingerprintToSDCard(char* username, int id){
    ESP_LOGI(LOG_TAG, "Saving Fingerprint ID Data, Username %s, ID %d", username, id);
    
    createEmptyJsonFileIfNotExists(FINGERPRINT_FILE_PATH);

    if (isFingerprintIdRegistered(id)){
        ESP_LOGE(LOG_TAG, "Fingerprint ID %d is already registered under another user!", id);
        return false;
    }

    // Open the file and then close it and create new Static in heap
    File file = SD.open(FINGERPRINT_FILE_PATH, FILE_READ);
    JsonDocument document;

    if(file){
        DeserializationError error = deserializeJson(document, file);
        if (error) {
            ESP_LOGE(LOG_TAG, "Failed to deserialize JSON: %s", error.c_str());
            file.close();
            return false;
        }
        file.close();
    }else{
        ESP_LOGE(LOG_TAG, "Error opening the File!, File %s", FINGERPRINT_FILE_PATH);
        return false;
    }

    // Check if there is already user under that name
    bool userFound = false;
    for (JsonObject user : document.as<JsonArray>()) {
        if(user["name"] == username){
            JsonArray fingerprintIds = user["key_access"].as<JsonArray>();
            fingerprintIds.add(id);
            userFound = true;

            ESP_LOGI(LOG_TAG, "Added new Fingerprint ID to existing user, Username %s, ID %d", username, id);
            break;
        }
    }
    
    // If user doesn't exist, create a new object for the user
    if (!userFound) {
        JsonObject newUser = document.add<JsonObject>();
        newUser["name"] = username;
        newUser["key_access"].as<JsonArray>().add(id);
        
        ESP_LOGI(LOG_TAG, "Created new user %s with Fingerprint ID: %d", username, id);
    }

    // Write the modified JSON data back to the SD card
    file = SD.open(FINGERPRINT_FILE_PATH, FILE_WRITE);
    if (file) {
        serializeJson(document, file);
        file.close();
        document.clear();

        ESP_LOGI(LOG_TAG, "Fingerprint data is successfully stored to SD Card");
        return true;
    } else {
        file.close();
        document.clear();

        ESP_LOGI(LOG_TAG, "Failed to store Fingerprint data to SD Card");
        return false;
    }
} 

bool SDCardModule::deleteFingerprintFromSDCard(char* username, int id){
    ESP_LOGI(LOG_TAG, "Delete Fingerprint ID Data, Username %s, ID %d", username, id);

    // Open the file and then close it and create new Static in heap
    File file = SD.open(FINGERPRINT_FILE_PATH, FILE_READ);
    JsonDocument document;

    if(file){
        DeserializationError error = deserializeJson(document, file);
        if (error) {
            ESP_LOGE(LOG_TAG, "Failed to deserialize JSON: %s", error.c_str());
            file.close();
            return false;
        }
        file.close();
    }else{
        ESP_LOGE(LOG_TAG, "Error opening the File!, File %s", FINGERPRINT_FILE_PATH);
        return false;
    }

    bool userFound = false;
    bool itemFound = false;

    for (JsonObject user : document.as<JsonArray>()) {
        const char* currentUsername = user["name"];

        if(user["name"] == username && id != 0){
            userFound = true;
            JsonArray fingerprintIds = user["key_access"].as<JsonArray>();

            // If a specific Fingerprint is provided, search for it and remove it
            for (int i = 0; i < fingerprintIds.size(); i++) {
                if (fingerprintIds[i].as<uint8_t>() == id) {
                    fingerprintIds.remove(i);
                    itemFound = true;
                    ESP_LOGI(LOG_TAG, "Removed Fingerprint ID: %d for User: %s", id, username);
                    break;
                }
            }
        }

        if (id == 0){
            userFound = true;
            itemFound = true;
            user.clear();
            ESP_LOGI(LOG_TAG, "Removed all Fingerprints Access for User %s", currentUsername);
        }
    }

    if (userFound && itemFound) return true;
    else return false;
}

bool SDCardModule::isNFCIdRegistered(char* id){
    ESP_LOGI(LOG_TAG, "Checking NFC ID %s in SD Card already exist", id);

    // Open the file and then close it and create new Static in heap
    File file = SD.open(RFID_FILE_PATH, FILE_READ);
    JsonDocument document;

    if(file){
        DeserializationError error = deserializeJson(document, file);
        if (error) {
            ESP_LOGE(LOG_TAG, "Failed to deserialize JSON: %s", error.c_str());
            file.close();
            return false;
        }
        file.close();
    }else{
        ESP_LOGE(LOG_TAG, "Error opening the File!, File %s", RFID_FILE_PATH);
        return false;
    }

    for (JsonObject user : document.as<JsonArray>()) {
        const char* currentUsername = user["name"];
        JsonArray nfcIds = user["key_access"].as<JsonArray>();

        for (int i = 0; i < nfcIds.size(); ++i) {
            const char* nfcId = nfcIds[i];
            if (nfcId == id) {
              ESP_LOGI(LOG_TAG, "NFC ID found %s under User %s", id, currentUsername);
              return true;
            }
        }
    }
    ESP_LOGW(LOG_TAG, "NFC ID %s not found in any user", id);
    return false;
}

bool SDCardModule::saveNFCToSDCard(char* username, char* id){
    ESP_LOGI(LOG_TAG, "Saving NFC Data, Username %s, ID %s", username, id);

    createEmptyJsonFileIfNotExists(RFID_FILE_PATH);

    if (isNFCIdRegistered(id)){
        ESP_LOGE(LOG_TAG, "NFC ID %s is already registered under another user!", id);
        return false;
    }

    // Open the file and then close it and create new Static in heap
    File file = SD.open(RFID_FILE_PATH, FILE_READ);
    JsonDocument document;

    if(file){
        DeserializationError error = deserializeJson(document, file);
        if (error) {
            ESP_LOGE(LOG_TAG, "Failed to deserialize JSON: %s", error.c_str());
            file.close();
            return false;
        }
        file.close();
    }else{
        ESP_LOGE(LOG_TAG, "Error opening the File!, File %s", RFID_FILE_PATH);
        return false;
    }

    // Check if there is already user under that name
    bool userFound = false;
    for (JsonObject user : document.as<JsonArray>()) {
        if(user["name"] == username){
            JsonArray nfcIds = user["key_access"].as<JsonArray>();
            nfcIds.add(id);
            userFound = true;

            ESP_LOGI(LOG_TAG, "Added new NFC ID to existing user, Username %s, NFC ID %s", username, id);
            break;
        }
    }

    // If user doesn't exist, create a new object for the user
    if (!userFound) {
        JsonObject newUser = document.add<JsonObject>();
        newUser["name"] = username;
        newUser["key_access"].as<JsonArray>().add(id);
        
        ESP_LOGI(LOG_TAG, "Created new user of %s with NFC ID: %s", username, id);
    }

    // Write the modified JSON data back to the SD card
    file = SD.open(RFID_FILE_PATH, FILE_WRITE);
    if (file) {
        serializeJson(document, file);
        file.close();
        document.clear();

        ESP_LOGI(LOG_TAG, "NFC data is successfully stored to SD Card");
        return true;
    } else {
        file.close();
        document.clear();

        ESP_LOGI(LOG_TAG, "Failed to store NFC data to SD Card");
        return false;
    }

}

bool SDCardModule::deleteNFCFromSDCard(char* username, char* id){
    ESP_LOGI(LOG_TAG, "Delete NFC ID Data, Username %s, ID %s", username, id);

    // Open the file and then close it and create new Static in heap
    File file = SD.open(RFID_FILE_PATH, FILE_READ);
    JsonDocument document;

    if(file){
        DeserializationError error = deserializeJson(document, file);
        if (error) {
            ESP_LOGE(LOG_TAG, "Failed to deserialize JSON: %s", error.c_str());
            file.close();
            return false;
        }
        file.close();
    }else{
        ESP_LOGE(LOG_TAG, "Error opening the File!, File %s", RFID_FILE_PATH);
        return false;
    }

    bool userFound = false;
    bool itemFound = false;

    for (JsonObject user : document.as<JsonArray>()) {
        const char* currentUsername = user["name"];

        if(user["name"] == username && id != 0){
            userFound = true;
            JsonArray nfcIds = user["key_access"].as<JsonArray>();

            // If a specific NFC is provided, search for it and remove it
            for (int i = 0; i < nfcIds.size(); i++) {
                const char* nfcId = nfcIds[i];

                if (nfcId == id) {
                    nfcIds.remove(i);
                    itemFound = true;
                    ESP_LOGI(LOG_TAG, "Removed NFC ID: %s for User: %s", id, username);
                    break;
                }
            }
        }

        if (id == 0){
            userFound = true;
            itemFound = true;
            user.clear();
            ESP_LOGI(LOG_TAG, "Removed all NFC Access for User %s", currentUsername);
        }
    }

    if (userFound && itemFound) return true;
    else return false;
}

void SDCardModule::createEmptyJsonFileIfNotExists(const char* filePath) {
    ESP_LOGI(LOG_TAG, "Start creating Empty JSON File");

    // Check if the file exists, if not, create it
    if (!SD.exists(filePath)) {
      ESP_LOGI(LOG_TAG, "File does not exist, creating a new one.");

      File file = SD.open(filePath, FILE_WRITE);    
      if (file) {
        JsonDocument doc;
  
        // Serialize the empty JSON object to the file
        if (serializeJson(doc, file) == 0) {
          ESP_LOGI(LOG_TAG, "Failed to write empty JSON object to the file");
        } else {
          ESP_LOGI(LOG_TAG, "Empty JSON object written to file successfully");
        }

        file.close();
      } else {
        ESP_LOGI(LOG_TAG, "Failed to open file for writing");
      }
    }else{
        ESP_LOGI(LOG_TAG, "There's already JSON file of %s available!", filePath);
    }
  }
