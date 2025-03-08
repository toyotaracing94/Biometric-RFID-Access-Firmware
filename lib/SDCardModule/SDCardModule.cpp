#define LOG_TAG "SD_CARD"

#include "esp_log.h"
#include "SDCardModule.h"
#include <ArduinoJson.h>

SDCardModule::SDCardModule() {
    setup();
}

bool SDCardModule::setup(){
    ESP_LOGD(LOG_TAG, "Initializing the SPI! SCK PIN %d, MISO PIN %d, CS_PIN %d", SCK_PIN, MISO_PIN, CS_PIN);
    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
    
    int retries = 1;
    const int maxRetries = 5;
    unsigned long backoffTime = 1000;
    
    while (retries <= maxRetries) {
        if (SD.begin(CS_PIN, SPI)) {
            ESP_LOGI(LOG_TAG, "SD Card has been found and ready!");
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

bool SDCardModule::saveFingerprintToSDCard(char* username, int id){
    ESP_LOGD(LOG_TAG, "Saving Fingerprint ID Data, Username %s, ID %d", username, id);
    
    createEmptyJsonFileIfNotExists(FINGERPRINT_FILE_PATH);

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
        document.createNestedObject();
        document["name"] = username;
        document["key_access"].as<JsonArray>().add(id);
        
        ESP_LOGI(LOG_TAG,"Created new user with Fingerprint ID: %s", username);
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
    ESP_LOGD(LOG_TAG, "Delete Fingerprint ID Data, Username %s, ID %d", username, id);

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
        if(user["name"] == username && id != 0){
            userFound = true;
            if (user.containsKey("key_access")) {
                JsonArray fingerprintArray = user["key_access"].as<JsonArray>();

                // If a specific Fingerprint is provided, search for it and remove it
                for (int i = 0; i < fingerprintArray.size(); i++) {
                  if (fingerprintArray[i].as<uint8_t>() == id) {
                    fingerprintArray.remove(i);
                    itemFound = true;
                    ESP_LOGI(LOG_TAG, "Removed Fingerprint ID: %d for User: %s", id, username);
                    break;
                  }
                }
            }
        }

        if (id == 0){
            userFound = true;
            itemFound = true;
            user.clear();
            ESP_LOGI(LOG_TAG, "Removed all Fingerprints Access");
        }
    }

    if (userFound && itemFound) return true;
    else return false;
}

void SDCardModule::createEmptyJsonFileIfNotExists(const char* filePath) {
    // Check if the file exists, if not, create it
    if (!SD.exists(filePath)) {
      ESP_LOGD(LOG_TAG, "File does not exist, creating a new one.");

      File file = SD.open(filePath, FILE_WRITE);
      if (file) {
        JsonDocument doc;
  
        // Serialize the empty JSON object to the file
        if (serializeJson(doc, file) == 0) {
          ESP_LOGD(LOG_TAG, "Failed to write empty JSON object to the file");
        } else {
          ESP_LOGD(LOG_TAG, "Empty JSON object written to file successfully");
        }

        file.close();
      } else {
        ESP_LOGD(LOG_TAG, "Failed to open file for writing");
      }
    }
  }
  