#define SD_CARD_LOG_TAG "SD_CARD"

#include "esp_log.h"
#include "SDCardModule.h"

SDCardModule::SDCardModule() {
    setup();
    createEmptyJsonFileIfNotExists(FINGERPRINT_FILE_PATH);
    createEmptyJsonFileIfNotExists(RFID_FILE_PATH);
}

/**
 * @brief Initializes the SD card module using SPI communication.
 *
 * Tries to initialize the SD card with retries and exponential backoff in case of failure.
 * Logs information about the SD card type, size, and storage status.
 *
 * @return true if SD card is successfully initialized, false otherwise.
 */
bool SDCardModule::setup() {
    ESP_LOGI(SD_CARD_LOG_TAG, "Start SD Card Module Setup!");
    ESP_LOGI(SD_CARD_LOG_TAG, "Initializing the SPI! SCK PIN %d, MISO PIN %d, MOSI PIN %d, CS_PIN %d", SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);

    int retries = 1;
    const int maxRetries = 5;
    unsigned long backoffTime = 1000;

    while (retries <= maxRetries) {
        if (SD.begin(CS_PIN, SPI)) {
            ESP_LOGI(SD_CARD_LOG_TAG, "SD Card has been found and ready!");

            uint8_t cardType = SD.cardType();
            ESP_LOGI(SD_CARD_LOG_TAG, "Card type %d", cardType);

            if (cardType == CARD_MMC)
                ESP_LOGI(SD_CARD_LOG_TAG, "SD card type: MMC");
            else if (cardType == CARD_SD)
                ESP_LOGI(SD_CARD_LOG_TAG, "SD card type: SDSC");
            else if (cardType == CARD_SDHC)
                ESP_LOGI(SD_CARD_LOG_TAG, "SD card type: SDHC");
            else
                ESP_LOGI(SD_CARD_LOG_TAG, "SD card type: Unknown");

            ESP_LOGI(SD_CARD_LOG_TAG, "SD Card Size: %lluMB", SD.cardSize() / (1024 * 1024));
            ESP_LOGI(SD_CARD_LOG_TAG, "Total space: %lluMB", SD.totalBytes() / (1024 * 1024));
            ESP_LOGI(SD_CARD_LOG_TAG, "Used space: %lluMB", SD.usedBytes() / (1024 * 1024));

            ESP_LOGI(SD_CARD_LOG_TAG, "SD Card storage initialized");
            return true;
        }
        else {
            ESP_LOGE(SD_CARD_LOG_TAG, "SD Card Module not found! Retrying %d", retries);
            retries++;
            delay(backoffTime);

            // Exponential backoff: double the wait time after each failure
            backoffTime *= 2;
        }
    }

    ESP.restart();
    return false;
}

/**
 * @brief Checks if a fingerprint ID is already registered in the SD card.
 *
 * @param id The fingerprint ID to check.
 * @return true if the fingerprint ID is already registered, false otherwise.
 */
bool SDCardModule::isFingerprintIdRegistered(int id) {
    ESP_LOGI(SD_CARD_LOG_TAG, "Checking if Fingerprint ID %d is already registered on the SD Card", id);

    // Open the file and create a new Static JsonDocument
    File file = SD.open(FINGERPRINT_FILE_PATH, FILE_READ);
    JsonDocument document;

    if (file) {
        DeserializationError error = deserializeJson(document, file);
        if (error) {
            ESP_LOGE(SD_CARD_LOG_TAG, "Failed to deserialize JSON: %s", error.c_str());
            file.close();
            return false;
        }
        file.close();
    } else {
        ESP_LOGE(SD_CARD_LOG_TAG, "Error opening the file: %s", FINGERPRINT_FILE_PATH);
        return false;
    }

    // Iterate through users
    for (JsonObject user : document.as<JsonArray>()) {
        const char *currentUsername = user["name"];

        // Iterate through fingerprints for the current user
        JsonArray fingerprints = user["fingerprints"].as<JsonArray>();
        for (JsonObject fingerprint : fingerprints) {
            // Check if the fingerprintId matches
            if (fingerprint["fingerprintId"].as<int>() == id) {
                ESP_LOGI(SD_CARD_LOG_TAG, "Fingerprint ID %d found under user %s", id, currentUsername);
                return true;
            }
        }
    }

    ESP_LOGW(SD_CARD_LOG_TAG, "Fingerprint ID %d not found in any user", id);
    return false;
}

/**
 * @brief Saves a fingerprint ID for a user to the SD card.
 *
 * Creates or updates a user with the given username and fingerprint ID.
 * If the fingerprint ID is already registered, returns false.
 *
 * @param username The username of the user.
 * @param fingerprintId The fingerprint ID to save.
 * @param keyAccessId The Key Access ID/Visitor ID that represent the fingerprint key access in the server
 * @return `true` if the fingerprint data was successfully saved, `false` otherwise.
 */
bool SDCardModule::saveFingerprintToSDCard(const char *username, int fingerprintId, const char *keyAccessId) {
    ESP_LOGI(SD_CARD_LOG_TAG, "Saving Fingerprint ID Data, Username %s, ID %d, KeyAccessId %s", username, fingerprintId, keyAccessId);
    createEmptyJsonFileIfNotExists(FINGERPRINT_FILE_PATH);

    if (isFingerprintIdRegistered(fingerprintId)) {
        ESP_LOGE(SD_CARD_LOG_TAG, "Fingerprint ID %d is already registered under another user!", fingerprintId);
        return false;
    }

    // Read the JSON from SD card
    File file = SD.open(FINGERPRINT_FILE_PATH, FILE_READ);
    JsonDocument document;

    if (file) {
        DeserializationError error = deserializeJson(document, file);
        if (error) {
            ESP_LOGE(SD_CARD_LOG_TAG, "Failed to deserialize JSON: %s", error.c_str());
            file.close();
            return false;
        }
        file.close();
    } else {
        ESP_LOGE(SD_CARD_LOG_TAG, "Error opening the file: %s", FINGERPRINT_FILE_PATH);
        return false;
    }

    // Search for existing user
    bool userFound = false;
    for (JsonObject user : document.as<JsonArray>()) {
        if (user["name"] == username) {
            JsonArray fingerprints = user["fingerprints"].as<JsonArray>();
            JsonObject newFingerprint = fingerprints.createNestedObject();
            newFingerprint["fingerprintId"] = fingerprintId;
            newFingerprint["keyAccessId"] = keyAccessId;

            userFound = true;
            ESP_LOGI(SD_CARD_LOG_TAG, "Added new Fingerprint ID to existing user, Username %s, ID %d", username, fingerprintId);
            break;
        }
    }

    // If user does not exist, create a new one
    if (!userFound) {
        JsonObject newUser = document.add<JsonObject>();
        newUser["name"] = username;
        JsonArray fingerprints = newUser.createNestedArray("fingerprints");

        JsonObject newFingerprint = fingerprints.createNestedObject();
        newFingerprint["fingerprintId"] = fingerprintId;
        newFingerprint["keyAccessId"] = keyAccessId;

        ESP_LOGI(SD_CARD_LOG_TAG, "Created new user %s with Fingerprint ID: %d", username, fingerprintId);
    }

    // Write back to file
    file = SD.open(FINGERPRINT_FILE_PATH, FILE_WRITE);
    if (file) {
        serializeJson(document, file);
        file.close();
        document.clear();

        ESP_LOGI(SD_CARD_LOG_TAG, "Fingerprint data successfully stored to SD Card");
        return true;
    } else {
        document.clear();
        ESP_LOGI(SD_CARD_LOG_TAG, "Failed to store Fingerprint data to SD Card");
        return false;
    }
}

/**
 * @brief Deletes a fingerprint ID from a user's record on the SD card.
 *
 * Searches for the given keyAccessId that associated the specified fingerprint ID.
 * If `id == 0`, all fingerprints of all the users are removed.
 *
 * @param keyAccessId The fingerprint ID to delete.　or `0` to delete allfingerprints models of the user
 * @return `true` if the fingerprint ID was successfully deleted, `false` otherwise.
 */
bool SDCardModule::deleteFingerprintFromSDCard(const char* keyAccessId) {
    ESP_LOGI(SD_CARD_LOG_TAG, "Deleting Fingerprint ID Data, KeyAccessId: %s", keyAccessId);

    if (strcmp(keyAccessId, "0") == 0) {
        // Delete all fingerprint data by creating an empty JSON array
        File file = SD.open(FINGERPRINT_FILE_PATH, FILE_WRITE);
        if (file) {
            // Write an empty JSON array to the file
            file.print("[]");
            file.close();
            ESP_LOGI(SD_CARD_LOG_TAG, "All fingerprint data deleted successfully from SD Card");
            return true;
        } else {
            ESP_LOGE(SD_CARD_LOG_TAG, "Error opening file for deletion of all data: %s", FINGERPRINT_FILE_PATH);
            return false;
        }
    }

    // Open the file and create new Static JsonDocument
    File file = SD.open(FINGERPRINT_FILE_PATH, FILE_READ);
    JsonDocument document;

    if (file) {
        DeserializationError error = deserializeJson(document, file);
        if (error) {
            ESP_LOGE(SD_CARD_LOG_TAG, "Failed to deserialize JSON: %s", error.c_str());
            file.close();
            return false;
        }
        file.close();
    } else {
        ESP_LOGE(SD_CARD_LOG_TAG, "Error opening the file: %s", FINGERPRINT_FILE_PATH);
        return false;
    }

    bool userFound = false;

    // Iterate over users and their fingerprints
    JsonArray users = document.as<JsonArray>();
    for (int i = 0; i < users.size(); i++) {
        JsonObject user = users[i].as<JsonObject>();

        // Check if the user has fingerprints
        JsonArray fingerprints = user["fingerprints"].as<JsonArray>();
        for (int j = 0; j < fingerprints.size(); j++) {
            JsonObject fingerprint = fingerprints[j].as<JsonObject>();

            // Compare the keyAccessId (fingerprint key) with the input
            if (fingerprint["keyAccessId"].as<String>() == keyAccessId) {
                fingerprints.remove(j);
                userFound = true;
                ESP_LOGI(SD_CARD_LOG_TAG, "Removed Fingerprint Access for User with KeyAccessId: %s", keyAccessId);
                break;
            }
        }
        if (userFound) {
            break;
        }
    }

    if (userFound) {
        // Write the modified JSON data back to the SD card
        file = SD.open(FINGERPRINT_FILE_PATH, FILE_WRITE);
        if (file) {
            if (serializeJson(document, file) == 0) {
                ESP_LOGE(SD_CARD_LOG_TAG, "Failed to serialize JSON to file");
                file.close();
                return false;
            }
            file.close();
            ESP_LOGI(SD_CARD_LOG_TAG, "Fingerprint data change is successfully stored to SD Card");
            return true;
        } else {
            ESP_LOGE(SD_CARD_LOG_TAG, "Failed to change Fingerprint data on SD Card");
            return false;
        }
    } else {
        ESP_LOGE(SD_CARD_LOG_TAG, "Fingerprint Data with KeyAccessId %s not found in Storage system!", keyAccessId);
        return false;
    }
}


/**
 * @brief Gets the Fingerprint ID associated with a given VisitorId.
 *
 * @param keyAccessId The Key Access ID to search for.
 * @return The Fingerprint ID if found, or -1 if not found.
 */
int SDCardModule::getFingerprintIdByVisitorId(const char* keyAccessId) {
    ESP_LOGI(SD_CARD_LOG_TAG, "Searching for Fingerprint ID by KeyAccessId: %s", keyAccessId);

    JsonDocument document;

    // Open the file containing fingerprint data
    File file = SD.open(FINGERPRINT_FILE_PATH, FILE_READ);

    if (!file) {
        ESP_LOGE(SD_CARD_LOG_TAG, "Error opening the file: %s", FINGERPRINT_FILE_PATH);
        return -1;
    }

    // Deserialize the JSON data
    DeserializationError error = deserializeJson(document, file);
    file.close();

    if (error) {
        ESP_LOGE(SD_CARD_LOG_TAG, "Failed to deserialize JSON: %s", error.c_str());
        return -1;
    }

    // Check if the document is a valid array
    if (!document.is<JsonArray>()) {
        ESP_LOGE(SD_CARD_LOG_TAG, "JSON file does not contain an array.");
        return -1;
    }

    // Search for the KeyAccessId in the "fingerprints" array of each user
    for (JsonObject user : document.as<JsonArray>()) {
        JsonArray fingerprints = user["fingerprints"].as<JsonArray>();
        
        // Iterate through fingerprints array to find the keyAccessId
        for (JsonObject fingerprint : fingerprints) {
            const char* currentKeyAccessId = fingerprint["keyAccessId"];
            
            // If the keyAccessId matches, return the associated fingerprintId
            if (strcmp(currentKeyAccessId, keyAccessId) == 0) {
                int fingerprintId = fingerprint["fingerprintId"].as<int>();
                ESP_LOGI(SD_CARD_LOG_TAG, "Fingerprint model found for KeyAccessId: %s, FingerprintId: %d", keyAccessId, fingerprintId);
                return fingerprintId;
            }
        }
    }

    ESP_LOGW(SD_CARD_LOG_TAG, "Fingerprint model with KeyAccessId: %s not found", keyAccessId);
    return -1;
}

/**
 * @brief Gets the Visitor ID that associated with the given fingerprint Id
 *
 * @param fingerprintId The Key Access ID to search for.
 * @return The Fingerprint ID if found, or -1 if not found.
 */
std::string* SDCardModule::getVisitorIdByFingerprintId(int fingerprintId) {
    ESP_LOGI(SD_CARD_LOG_TAG, "Get Visitor ID by Fingerprint ID %d in SD Card", fingerprintId);

    // Open the file and then close it and create new Static in heap
    File file = SD.open(FINGERPRINT_FILE_PATH, FILE_READ);
    JsonDocument document;

    if (file) {
        DeserializationError error = deserializeJson(document, file);
        if (error) {
            ESP_LOGE(SD_CARD_LOG_TAG, "Failed to deserialize JSON: %s", error.c_str());
            file.close();
            return nullptr;
        }
        file.close();
    } else {
        ESP_LOGE(SD_CARD_LOG_TAG, "Error opening the File!, File %s", FINGERPRINT_FILE_PATH);
        return nullptr;
    }

    // Loop through all users in the document
    for (JsonObject user : document.as<JsonArray>()) {
        const char *currentVisitorId = user["visitor_id"];

        // Check if the user has a visitor ID
        if (currentVisitorId == nullptr) {
            continue;
        }

        // Get the key_access array (which holds the fingerprintIds)
        JsonArray fingerprintIds = user["key_access"].as<JsonArray>();

        // Loop through the fingerprintIds array to find the matching fingerprintId
        for (int id : fingerprintIds) {
            if (id == fingerprintId) {
                ESP_LOGI(SD_CARD_LOG_TAG, "Fingerprint ID %d found for Visitor ID %s", fingerprintId, currentVisitorId);
                return new std::string(currentVisitorId);
            }
        }
    }

    // If no matching fingerprintId is found, log and return nullptr
    ESP_LOGW(SD_CARD_LOG_TAG, "Fingerprint ID %d not found in any user", fingerprintId);
    return nullptr;
}

/**
 * @brief Checks if a specific NFC ID is already registered in the SD card.
 *
 * @param id The NFC ID to check.
 * @return `true` if the NFC ID is already registered, `false` otherwise.
 */
bool SDCardModule::isNFCIdRegistered(const char *id) {
    ESP_LOGI(SD_CARD_LOG_TAG, "Checking if NFC ID %s already exists in SD Card", id);

    // Open the file and then close it and create new Static in heap
    File file = SD.open(RFID_FILE_PATH, FILE_READ);
    JsonDocument document;

    if (file) {
        DeserializationError error = deserializeJson(document, file);
        if (error) {
            ESP_LOGE(SD_CARD_LOG_TAG, "Failed to deserialize JSON: %s", error.c_str());
            file.close();
            return false;
        }
        file.close();
    } else {
        ESP_LOGE(SD_CARD_LOG_TAG, "Error opening the file: %s", RFID_FILE_PATH);
        return false;
    }

    // Loop through each user in the JSON array
    for (JsonObject user : document.as<JsonArray>()) {
        const char *currentUsername = user["name"];

        // Get the nfcs array for the current user
        JsonArray nfcs = user["nfcs"].as<JsonArray>();

        // Loop through each NFC entry for the current user
        for (JsonObject nfc : nfcs) {
            const char *nfcId = nfc["nfcId"];

            // Check if the current NFC ID matches the one we're looking for
            if (strcmp(nfcId, id) == 0) {
                ESP_LOGI(SD_CARD_LOG_TAG, "NFC ID %s found under User %s", id, currentUsername);
                return true;
            }
        }
    }

    ESP_LOGW(SD_CARD_LOG_TAG, "NFC ID %s not found in any user", id);
    return false;
}

/**
 * @brief Saves an NFC ID for a user to the SD card.
 *
 * @param username The username of the user.
 * @param nfcId The NFC ID to save.
 * @param visitorId The Visitor ID/Key Access Id to save.
 * @return `true` if the NFC data was successfully saved, `false` otherwise.
 */
bool SDCardModule::saveNFCToSDCard(const char *username, const char *nfcId, const char *visitorId) {
    ESP_LOGI(SD_CARD_LOG_TAG, "Saving NFC Data, Username %s, NFC ID %s", username, nfcId);

    createEmptyJsonFileIfNotExists(RFID_FILE_PATH);

    if (isNFCIdRegistered(nfcId)) {
        ESP_LOGE(SD_CARD_LOG_TAG, "NFC ID %s is already registered under another user!", nfcId);
        return false;
    }

    // Open the file and then close it and create new Static in heap
    File file = SD.open(RFID_FILE_PATH, FILE_READ);
    JsonDocument document;

    if (file) {
        DeserializationError error = deserializeJson(document, file);
        if (error) {
            ESP_LOGE(SD_CARD_LOG_TAG, "Failed to deserialize JSON: %s", error.c_str());
            file.close();
            return false;
        }
        file.close();
    } else {
        ESP_LOGE(SD_CARD_LOG_TAG, "Error opening the File!, File %s", RFID_FILE_PATH);
        return false;
    }

    // Check if there is already a user with that name
    bool userFound = false;
    for (JsonObject user : document.as<JsonArray>()) {
        if (user["name"] == username) {
            // If the user is found, add the new NFC info to the 'nfcs' array
            JsonArray nfcs = user["nfcs"].as<JsonArray>();
            JsonObject newNfc = nfcs.createNestedObject();
            newNfc["nfcId"] = nfcId;
            newNfc["keyAccessId"] = visitorId;
            userFound = true;

            ESP_LOGI(SD_CARD_LOG_TAG, "Added new NFC ID to existing user, Username %s, NFC ID %s", username, nfcId);
            break;
        }
    }

    // If user doesn't exist, create a new object for the user
    if (!userFound) {
        JsonObject newUser = document.add<JsonObject>();
        newUser["name"] = username;
        newUser["visitor_id"] = visitorId;

        // Create the 'nfcs' array for this new user
        JsonArray nfcs = newUser.createNestedArray("nfcs");
        JsonObject newNfc = nfcs.createNestedObject();
        newNfc["nfcId"] = nfcId;
        newNfc["keyAccessId"] = visitorId;

        ESP_LOGI(SD_CARD_LOG_TAG, "Created new user %s with NFC ID: %s", username, nfcId);
    }

    // Write the modified JSON data back to the SD card
    file = SD.open(RFID_FILE_PATH, FILE_WRITE);
    if (file) {
        serializeJson(document, file);
        file.close();
        document.clear();

        ESP_LOGI(SD_CARD_LOG_TAG, "NFC data is successfully stored to SD Card");
        return true;
    } else {
        file.close();
        document.clear();

        ESP_LOGI(SD_CARD_LOG_TAG, "Failed to store NFC data to SD Card");
        return false;
    }
}


/**
 * @brief Deletes an NFC Key access from a user's record on the SD card by VisitorId
 *
 * Searches for the given user and removes the specified NFC ID.
 * If `id == 0`, all NFC IDs of the user are removed.
 *
 * @param visitorId The NFC Visitor ID to delete. or `0` to delete all NFC access of the user
 * @return `true` if the NFC ID was successfully deleted, `false` otherwise.
 */
bool SDCardModule::deleteNFCFromSDCard(const char *visitorId) {
    ESP_LOGI(SD_CARD_LOG_TAG, "Delete NFC Data, Visitor ID %s", visitorId);

    if (strcmp(visitorId, "0") == 0) {
        // Delete all NFC data by creating an empty JSON array
        File file = SD.open(RFID_FILE_PATH, FILE_WRITE);
        if (file) {
            // Write an empty JSON array to the file
            file.print("[]");
            file.close();
            ESP_LOGI(SD_CARD_LOG_TAG, "All NFC data deleted successfully from SD Card");
            return true;
        } else {
            ESP_LOGE(SD_CARD_LOG_TAG, "Error opening file for deletion of all data: %s", RFID_FILE_PATH);
            return false;
        }
    }

    // Open the file for reading the data
    File file = SD.open(RFID_FILE_PATH, FILE_READ);
    JsonDocument document;

    if (file) {
        DeserializationError error = deserializeJson(document, file);
        if (error) {
            ESP_LOGE(SD_CARD_LOG_TAG, "Failed to deserialize JSON: %s", error.c_str());
            file.close();
            return false;
        }
        file.close();
    } else {
        ESP_LOGE(SD_CARD_LOG_TAG, "Error opening the File!, File %s", RFID_FILE_PATH);
        return false;
    }

    bool userFound = false;
    bool nfcFound = false;

    // Loop through each user in the JSON array
    for (JsonObject user : document.as<JsonArray>()) {
        const char *currentVisitorId = user["visitor_id"];
        if (currentVisitorId == nullptr || strcmp(currentVisitorId, visitorId) != 0) {
            // Skip if the visitor ID doesn't match
            continue;
        }

        userFound = true;

        // Loop through the NFCs for the current user
        JsonArray nfcs = user["nfcs"].as<JsonArray>();
        for (int i = 0; i < nfcs.size(); i++) {
            JsonObject nfc = nfcs[i].as<JsonObject>();
            const char *nfcId = nfc["nfcId"];

            // If the NFC ID exists, remove it from the array
            if (nfcId != nullptr) {
                nfcFound = true;
                nfcs.remove(i);
                ESP_LOGI(SD_CARD_LOG_TAG, "Removed NFC Access with NFC ID: %s", nfcId);
                break;
            }
        }
        if (!nfcFound) {
            ESP_LOGW(SD_CARD_LOG_TAG, "No NFC data found for Visitor ID %s", visitorId);
        }
        break;
    }

    // If we found the user and NFC entry to delete
    if (userFound && nfcFound) {
        // Write the modified JSON data back to the SD card
        file = SD.open(RFID_FILE_PATH, FILE_WRITE);
        if (file) {
            serializeJson(document, file);
            file.close();
            document.clear();

            ESP_LOGI(SD_CARD_LOG_TAG, "NFC data successfully updated in SD Card");
            return true;
        } else {
            file.close();
            document.clear();

            ESP_LOGE(SD_CARD_LOG_TAG, "Failed to update NFC data in SD Card");
            return false;
        }
    } else {
        ESP_LOGE(SD_CARD_LOG_TAG, "Visitor ID %s not found or no NFC data to remove", visitorId);
        return false;
    }
}

/**
 * @brief Get the visitor Id that match with the NFC UID
 *
 * Searches for the given visitor ID that match with the NFC UID Card that was read/pass to the function
 *
 * @param id The NFC UID Card
 * @return std::string Visitor ID of the NFC Card
 */
std::string* SDCardModule::getVisitorIdByNFC(char *id) {
    ESP_LOGI(SD_CARD_LOG_TAG, "Get Visitor ID by NFC ID %s in SD Card", id);

    // Open the file and then close it and create new Static in heap
    File file = SD.open(RFID_FILE_PATH, FILE_READ);
    JsonDocument document;

    if (file) {
        DeserializationError error = deserializeJson(document, file);
        if (error) {
            ESP_LOGE(SD_CARD_LOG_TAG, "Failed to deserialize JSON: %s", error.c_str());
            file.close();
            return nullptr;
        }
        file.close();
    } else {
        ESP_LOGE(SD_CARD_LOG_TAG, "Error opening the File!, File %s", RFID_FILE_PATH);
        return nullptr;
    }

    // Loop through all users in the document
    for (JsonObject user : document.as<JsonArray>()) {
        const char *currentVisitorId = user["visitor_id"];
        
        // Check if the user has a visitor ID
        if (currentVisitorId == nullptr) {
            continue;
        }

        // Get the nfcs array for the user
        JsonArray nfcs = user["nfcs"].as<JsonArray>();
        
        // Loop through the nfcs array to find the matching nfcId
        for (JsonObject nfc : nfcs) {
            const char *nfcId = nfc["nfcId"];
            
            // Check if the nfcId matches the one we are looking for
            if (nfcId != nullptr && strcmp(nfcId, id) == 0) {
                ESP_LOGI(SD_CARD_LOG_TAG, "NFC ID %s found for Visitor ID %s", id, currentVisitorId);
                return new std::string(currentVisitorId);
            }
        }
    }

    // If no matching NFC ID is found, log and return nullptr
    ESP_LOGW(SD_CARD_LOG_TAG, "NFC ID %s not found in any user", id);
    return nullptr;
}

/**
 * @brief Ensures that an empty JSON file exists at the specified path. If there isn't, create the new object in there
 *
 * @param filePath The path to the JSON file to create.
 */
void SDCardModule::createEmptyJsonFileIfNotExists(const char *filePath){
    ESP_LOGI(SD_CARD_LOG_TAG, "Start creating Empty JSON File");

    // Check if the file exists, if not, create it
    if (!SD.exists(filePath)){
        ESP_LOGI(SD_CARD_LOG_TAG, "File does not exist, creating a new one.");

        File file = SD.open(filePath, FILE_WRITE);
        if (file){
            JsonDocument doc;

            // Serialize the empty JSON object to the file
            if (serializeJson(doc, file) == 0){
                ESP_LOGI(SD_CARD_LOG_TAG, "Failed to write empty JSON object to the file");
            }
            else{
                ESP_LOGI(SD_CARD_LOG_TAG, "Empty JSON object written to file successfully");
            }

            file.close();
        }
        else ESP_LOGI(SD_CARD_LOG_TAG, "Failed to open file for writing");
    }
    else ESP_LOGI(SD_CARD_LOG_TAG, "There's already JSON file of %s available!", filePath);
}

/**
 * @brief Synchronizes RFID and Fingerprint data from SD card to a JsonObject.
 *
 * @return JsonObject The synchronized data from the RFID and Fingerprint files.
 */
JsonDocument SDCardModule::syncData(){
    ESP_LOGI(SD_CARD_LOG_TAG, "Start sync data from ESP32 SD Card to Titan");

    JsonDocument document;
    JsonObject data = document.to<JsonObject>();
    const char *filePaths[] = {RFID_FILE_PATH, FINGERPRINT_FILE_PATH};

    // For future dev!
    // TODO : I'm lazy to do this in proper way, and I don't know when will it scale but for now this will do
    for (int i = 0; i < 2; i++){
        const char *filePath = filePaths[i];
        ESP_LOGI(SD_CARD_LOG_TAG, "Reading file: %s", filePath);

        // Open the file from SD card
        File file = SD.open(filePath, FILE_READ);
        if (!file){
            ESP_LOGI(SD_CARD_LOG_TAG, "Failed to open file: %s", filePath);
            continue;
        }

        // Use a smaller JsonDocument for each file read
        JsonDocument dataEachFile;
        DeserializationError error = deserializeJson(dataEachFile, file);
        if (error){
            ESP_LOGI(SD_CARD_LOG_TAG, "Failed to read file: %s", filePath);
            file.close();
            continue;
        }

        // Log the deserialized data to confirm it's not empty
        String buffer;
        serializeJson(dataEachFile, buffer);
        ESP_LOGI(SD_CARD_LOG_TAG, "Deserialized Data: %s", buffer.c_str());

        data[filePath] = dataEachFile;
        file.close();
    }

    return document;
}
