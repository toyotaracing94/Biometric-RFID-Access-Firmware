#define SD_CARD_LOG_TAG "SD_CARD"

#include "esp_log.h"
#include "SDCardModule.h"

SDCardModule::SDCardModule()
{
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
bool SDCardModule::setup()
{
    ESP_LOGI(SD_CARD_LOG_TAG, "Start SD Card Module Setup!");
    ESP_LOGI(SD_CARD_LOG_TAG, "Initializing the SPI! SCK PIN %d, MISO PIN %d, MOSI PIN %d, CS_PIN %d", SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);

    int retries = 1;
    const int maxRetries = 5;
    unsigned long backoffTime = 1000;

    while (retries <= maxRetries)
    {
        if (SD.begin(CS_PIN, SPI))
        {
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
        else
        {
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
bool SDCardModule::isFingerprintIdRegistered(int id)
{
    ESP_LOGI(SD_CARD_LOG_TAG, "Checking Fingerprint Model with ID %d in SD Card already exist", id);

    // Open the file and then close it and create new Static in heap
    File file = SD.open(FINGERPRINT_FILE_PATH, FILE_READ);
    JsonDocument document;

    if (file)
    {
        DeserializationError error = deserializeJson(document, file);
        if (error)
        {
            ESP_LOGE(SD_CARD_LOG_TAG, "Failed to deserialize JSON: %s", error.c_str());
            file.close();
            return false;
        }
        file.close();
    }
    else
    {
        ESP_LOGE(SD_CARD_LOG_TAG, "Error opening the File!, File %s", FINGERPRINT_FILE_PATH);
        return false;
    }

    for (JsonObject user : document.as<JsonArray>())
    {
        const char *currentUsername = user["name"];

        JsonArray fingerprintIds = user["key_access"].as<JsonArray>();

        for (int i = 0; i < fingerprintIds.size(); ++i)
        {
            if (fingerprintIds[i].as<uint8_t>() == id)
            {
                ESP_LOGI(SD_CARD_LOG_TAG, "Fingerprint ID found %d under User %s", id, currentUsername);
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
 * @param id The fingerprint ID to save.
 * @return true if the fingerprint data was successfully saved, false otherwise.
 */
bool SDCardModule::saveFingerprintToSDCard(const char *username, int id, const char *visitorId)
{
    ESP_LOGI(SD_CARD_LOG_TAG, "Saving Fingerprint ID Data, Username %s, ID %d, Visitor ID %s", username, id, visitorId);

    createEmptyJsonFileIfNotExists(FINGERPRINT_FILE_PATH);

    if (isFingerprintIdRegistered(id))
    {
        ESP_LOGE(SD_CARD_LOG_TAG, "Fingerprint ID %d is already registered under another user!", id);
        return false;
    }

    // Open the file and then close it and create new Static in heap
    File file = SD.open(FINGERPRINT_FILE_PATH, FILE_READ);
    JsonDocument document;

    if (file)
    {
        DeserializationError error = deserializeJson(document, file);
        if (error)
        {
            ESP_LOGE(SD_CARD_LOG_TAG, "Failed to deserialize JSON: %s", error.c_str());

            file.close();
            return false;
        }
        file.close();
    }
    else
    {
        ESP_LOGE(SD_CARD_LOG_TAG, "Error opening the File!, File %s", FINGERPRINT_FILE_PATH);
        return false;
    }

    // Check if there is already user under that name
    bool userFound = false;
    for (JsonObject user : document.as<JsonArray>())
    {
        if (user["name"] == username)
        {
            JsonArray fingerprintIds = user["key_access"].as<JsonArray>();
            fingerprintIds.add(id);
            userFound = true;

            ESP_LOGI(SD_CARD_LOG_TAG, "Added new Fingerprint ID to existing user, Username %s, ID %d", username, id);
            break;
        }
    }

    // If user doesn't exist, create a new object for the user
    if (!userFound)
    {
        JsonObject newUser = document.add<JsonObject>();
        newUser["name"] = username;
        newUser["visitor_id"] = visitorId;
        newUser["key_access"].to<JsonArray>().add(id);

        ESP_LOGI(SD_CARD_LOG_TAG, "Created new user %s with Fingerprint ID: %d", username, id);
    }

    // Write the modified JSON data back to the SD card
    file = SD.open(FINGERPRINT_FILE_PATH, FILE_WRITE);
    if (file)
    {
        serializeJson(document, file);
        file.close();
        document.clear();

        ESP_LOGI(SD_CARD_LOG_TAG, "Fingerprint data is successfully stored to SD Card");
        return true;
    }
    else
    {
        file.close();
        document.clear();

        ESP_LOGI(SD_CARD_LOG_TAG, "Failed to store Fingerprint data to SD Card");
        return false;
    }
}
bool SDCardModule::getFingerprintModelByVisitorId(const char *visitorId, JsonObject &result)
{
    ESP_LOGI(SD_CARD_LOG_TAG, "Searching for Fingerprint Model with Visitor ID: %s", visitorId);

    static StaticJsonDocument<4096> document; // Keeping the document in static memory

    // Open the file containing fingerprint data
    File file = SD.open(FINGERPRINT_FILE_PATH, FILE_READ);

    if (!file)
    {
        ESP_LOGE(SD_CARD_LOG_TAG, "Error opening the file: %s", FINGERPRINT_FILE_PATH);
        return false; // Return false if the file does not exist
    }

    // Deserialize the JSON data
    DeserializationError error = deserializeJson(document, file);
    file.close();

    if (error)
    {
        ESP_LOGE(SD_CARD_LOG_TAG, "Failed to deserialize JSON: %s", error.c_str());
        return false; // Return false on deserialization error
    }

    // Check if the document is a valid array
    if (!document.is<JsonArray>())
    {
        ESP_LOGE(SD_CARD_LOG_TAG, "JSON file does not contain an array.");
        return false; // Return false if not an array
    }

    // Search for the user by visitorId
    for (JsonObject user : document.as<JsonArray>())
    {
        if (!user.containsKey("visitor_id"))
        {
            continue; // Skip if the user object does not have "visitor_id"
        }

        const char *currentVisitorId = user["visitor_id"];
        if (strcmp(currentVisitorId, visitorId) == 0)
        {
            ESP_LOGI(SD_CARD_LOG_TAG, "Fingerprint model found for Visitor ID: %s", visitorId);
            result = user; // Copy the found user to the result parameter
            return true;   // Return true if match found
        }
    }

    ESP_LOGW(SD_CARD_LOG_TAG, "Fingerprint model with Visitor ID: %s not found", visitorId);
    return false; // Return false if no match found
}

JsonObject SDCardModule::getFingerprintModelById(int fingerprintId)
{
    ESP_LOGI(SD_CARD_LOG_TAG, "Searching for Fingerprint Model with Fingerprint ID: %d", fingerprintId);

    StaticJsonDocument<4096> document; // Adjust size according to your file size

    // Open the file containing fingerprint data
    File file = SD.open(FINGERPRINT_FILE_PATH, FILE_READ);

    if (!file)
    {
        ESP_LOGE(SD_CARD_LOG_TAG, "Error opening the file: %s", FINGERPRINT_FILE_PATH);
        return JsonObject(); // Return an empty JsonObject if the file does not exist
    }

    // Deserialize the JSON data
    DeserializationError error = deserializeJson(document, file);
    file.close();

    if (error)
    {
        ESP_LOGE(SD_CARD_LOG_TAG, "Failed to deserialize JSON: %s", error.c_str());
        return JsonObject(); // Return an empty JsonObject on deserialization error
    }

    // Check if the document is a valid array
    if (!document.is<JsonArray>())
    {
        ESP_LOGE(SD_CARD_LOG_TAG, "JSON file does not contain an array.");
        return JsonObject(); // Return an empty JsonObject if not an array
    }

    // Search for the fingerprintId in the "key_access" array of each user
    for (JsonObject user : document.as<JsonArray>())
    {
        if (!user.containsKey("key_access"))
        {
            continue; // Skip if the user object does not have "key_access"
        }

        JsonArray fingerprintIds = user["key_access"].as<JsonArray>();

        for (int id : fingerprintIds)
        {
            if (id == fingerprintId)
            {
                ESP_LOGI(SD_CARD_LOG_TAG, "Fingerprint model found for Fingerprint ID: %d", fingerprintId);
                return user; // Return the matched user object
            }
        }
    }

    ESP_LOGW(SD_CARD_LOG_TAG, "Fingerprint model with Fingerprint ID: %d not found", fingerprintId);
    return JsonObject(); // Return an empty JsonObject if the fingerprintId is not found
}

/**
 * @brief Deletes a fingerprint ID from a user's record on the SD card.
 *
 * Searches for the given user and removes the specified fingerprint ID.
 * If `id == 0`, all fingerprints of the user are removed.
 *
 * @param username The username of the user.
 * @param id The fingerprint ID to delete.　or `0` to delete allfingerprints models of the user
 * @return true if the fingerprint ID was successfully deleted, false otherwise.
 */
bool SDCardModule::deleteFingerprintFromSDCard(const char *visitorId)
{
    ESP_LOGI(SD_CARD_LOG_TAG, "Delete Fingerprint ID Data, VisitoID %s", visitorId);
    if (strcmp(visitorId, "0") == 0)
    {
        // Delete all fingerprint data by creating an empty JSON array
        File file = SD.open(FINGERPRINT_FILE_PATH, FILE_WRITE);
        if (file)
        {
            // Write an empty JSON array to the file
            file.print("[]");
            file.close();
            ESP_LOGI(SD_CARD_LOG_TAG, "All fingerprint data deleted successfully from SD Card");
            return true;
        }
        else
        {
            ESP_LOGE(SD_CARD_LOG_TAG, "Error opening file for deletion of all data: %s", FINGERPRINT_FILE_PATH);
            return false;
        }
    }
    // Open the file and then close it and create new Static in heap
    File file = SD.open(FINGERPRINT_FILE_PATH, FILE_READ);
    JsonDocument document;

    if (file)
    {
        DeserializationError error = deserializeJson(document, file);
        if (error)
        {
            ESP_LOGE(SD_CARD_LOG_TAG, "Failed to deserialize JSON: %s", error.c_str());
            file.close();
            return false;
        }
        file.close();
    }
    else
    {
        ESP_LOGE(SD_CARD_LOG_TAG, "Error opening the File!, File %s", FINGERPRINT_FILE_PATH);
        return false;
    }

    bool userFound = false;
    bool itemFound = false;

    for (JsonObject user : document.as<JsonArray>())
    {
        const char *visitorId = user["visitor_id"];
        const char *name = user["name"];
        if (user["visitor_id"] == visitorId)
        {
            userFound = true;
            itemFound = true;
            user.clear();
            ESP_LOGI(SD_CARD_LOG_TAG, "Removed all Fingerprints Access for User %s", name);
        }
    }

    if (userFound && itemFound)
    {
        // Write the modified JSON data back to the SD card
        file = SD.open(FINGERPRINT_FILE_PATH, FILE_WRITE);
        if (file)
        {
            serializeJson(document, file);
            file.close();
            document.clear();

            ESP_LOGI(SD_CARD_LOG_TAG, "Fingerprint data change is successfully stored to SD Card");
            return true;
        }
        else
        {
            file.close();
            document.clear();

            ESP_LOGI(SD_CARD_LOG_TAG, "Failed to change Fingerprint data to SD Card");
            return false;
        }
    }
    else
    {
        ESP_LOGE(SD_CARD_LOG_TAG, "Fingerprint Data Visitor ID %s  not found in Storage system!", visitorId);
        return false;
    }
}

/**
 * @brief Checks if a specific NFC ID is already registered in the SD card.
 *
 * @param id The NFC ID to check.
 * @return true if the NFC ID is already registered, false otherwise.
 */
bool SDCardModule::isNFCIdRegistered(char *id)
{
    ESP_LOGI(SD_CARD_LOG_TAG, "Checking NFC ID %s in SD Card already exist", id);

    // Open the file and then close it and create new Static in heap
    File file = SD.open(RFID_FILE_PATH, FILE_READ);
    JsonDocument document;

    if (file)
    {
        DeserializationError error = deserializeJson(document, file);
        if (error)
        {
            ESP_LOGE(SD_CARD_LOG_TAG, "Failed to deserialize JSON: %s", error.c_str());
            file.close();
            return false;
        }
        file.close();
    }
    else
    {
        ESP_LOGE(SD_CARD_LOG_TAG, "Error opening the File!, File %s", RFID_FILE_PATH);
        return false;
    }

    for (JsonObject user : document.as<JsonArray>())
    {
        const char *currentUsername = user["name"];
        JsonArray nfcIds = user["key_access"].as<JsonArray>();

        for (int i = 0; i < nfcIds.size(); ++i)
        {
            const char *nfcId = nfcIds[i];
            if (strcmp(nfcId, id) == 0)
            {
                ESP_LOGI(SD_CARD_LOG_TAG, "NFC ID found %s under User %s", id, currentUsername);
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
 * @param id The NFC ID to save.
 * @return true if the NFC data was successfully saved, false otherwise.
 */
bool SDCardModule::saveNFCToSDCard(const char *username, char *id, const char *visitorId)
{
    ESP_LOGI(SD_CARD_LOG_TAG, "Saving NFC Data, Username %s, ID %s", username, id);

    createEmptyJsonFileIfNotExists(RFID_FILE_PATH);

    if (isNFCIdRegistered(id))
    {
        ESP_LOGE(SD_CARD_LOG_TAG, "NFC ID %s is already registered under another user!", id);
        return false;
    }

    // Open the file and then close it and create new Static in heap
    File file = SD.open(RFID_FILE_PATH, FILE_READ);
    JsonDocument document;

    if (file)
    {
        DeserializationError error = deserializeJson(document, file);
        if (error)
        {
            ESP_LOGE(SD_CARD_LOG_TAG, "Failed to deserialize JSON: %s", error.c_str());
            file.close();
            return false;
        }
        file.close();
    }
    else
    {
        ESP_LOGE(SD_CARD_LOG_TAG, "Error opening the File!, File %s", RFID_FILE_PATH);
        return false;
    }

    // Check if there is already user under that name
    bool userFound = false;
    for (JsonObject user : document.as<JsonArray>())
    {
        if (user["name"] == username)
        {
            JsonArray nfcIds = user["key_access"].as<JsonArray>();
            nfcIds.add(id);
            userFound = true;

            ESP_LOGI(SD_CARD_LOG_TAG, "Added new NFC ID to existing user, Username %s, NFC ID %s", username, id);
            break;
        }
    }

    // If user doesn't exist, create a new object for the user
    if (!userFound)
    {
        JsonObject newUser = document.add<JsonObject>();
        newUser["name"] = username;
        newUser["visitor_id"] = visitorId;
        newUser["key_access"].to<JsonArray>().add(id);

        ESP_LOGI(SD_CARD_LOG_TAG, "Created new user of %s with NFC ID: %s", username, id);
    }

    // Write the modified JSON data back to the SD card
    file = SD.open(RFID_FILE_PATH, FILE_WRITE);
    if (file)
    {
        serializeJson(document, file);
        file.close();
        document.clear();

        ESP_LOGI(SD_CARD_LOG_TAG, "NFC data is successfully stored to SD Card");
        return true;
    }
    else
    {
        file.close();
        document.clear();

        ESP_LOGI(SD_CARD_LOG_TAG, "Failed to store NFC data to SD Card");
        return false;
    }
}

/**
 * @brief Deletes an NFC ID from a user's record on the SD card.
 *
 * Searches for the given user and removes the specified NFC ID.
 * If `id == 0`, all NFC IDs of the user are removed.
 *
 * @param username The username of the user.
 * @param id The NFC ID to delete. or `0` to delete all NFC access of the user
 * @return true if the NFC ID was successfully deleted, false otherwise.
 */
bool SDCardModule::deleteNFCFromSDCard(const char *username, const char *id)
{
    ESP_LOGI(SD_CARD_LOG_TAG, "Delete NFC ID Data, Username %s, ID %s", username, id);

    // Open the file and then close it and create new Static in heap
    File file = SD.open(RFID_FILE_PATH, FILE_READ);
    JsonDocument document;

    if (file)
    {
        DeserializationError error = deserializeJson(document, file);
        if (error)
        {
            ESP_LOGE(SD_CARD_LOG_TAG, "Failed to deserialize JSON: %s", error.c_str());
            file.close();
            return false;
        }
        file.close();
    }
    else
    {
        ESP_LOGE(SD_CARD_LOG_TAG, "Error opening the File!, File %s", RFID_FILE_PATH);
        return false;
    }

    bool userFound = false;
    bool itemFound = false;

    for (JsonObject user : document.as<JsonArray>())
    {
        const char *currentUsername = user["name"];

        if (user["name"] == username && id != 0)
        {
            userFound = true;
            JsonArray nfcIds = user["key_access"].as<JsonArray>();

            // If a specific NFC is provided, search for it and remove it
            for (int i = 0; i < nfcIds.size(); i++)
            {
                const char *nfcId = nfcIds[i];

                if (strcmp(nfcId, id) == 0)
                {
                    nfcIds.remove(i);
                    itemFound = true;
                    ESP_LOGI(SD_CARD_LOG_TAG, "Removed NFC ID: %s for User: %s", id, username);
                    break;
                }
            }
        }

        if (id == 0)
        {
            userFound = true;
            itemFound = true;
            user.clear();
            ESP_LOGI(SD_CARD_LOG_TAG, "Removed all NFC Access for User %s", currentUsername);
        }
    }

    if (userFound && itemFound)
    {
        // Write the modified JSON data back to the SD card
        file = SD.open(RFID_FILE_PATH, FILE_WRITE);
        if (file)
        {
            serializeJson(document, file);
            file.close();
            document.clear();

            ESP_LOGI(SD_CARD_LOG_TAG, "NFC data is successfully stored to SD Card");
            return true;
        }
        else
        {
            file.close();
            document.clear();

            ESP_LOGI(SD_CARD_LOG_TAG, "Failed to store NFC data to SD Card");
            return false;
        }
    }
    else
    {
        ESP_LOGE(SD_CARD_LOG_TAG, "NFC Data ID %s with User %s not found in Storage system!", id, username);
        return false;
    }
}

/**
 * @brief Ensures that an empty JSON file exists at the specified path. If there isn't, create the new object in there
 *
 * @param filePath The path to the JSON file to create.
 */
void SDCardModule::createEmptyJsonFileIfNotExists(const char *filePath)
{
    ESP_LOGI(SD_CARD_LOG_TAG, "Start creating Empty JSON File");

    // Check if the file exists, if not, create it
    if (!SD.exists(filePath))
    {
        ESP_LOGI(SD_CARD_LOG_TAG, "File does not exist, creating a new one.");

        File file = SD.open(filePath, FILE_WRITE);
        if (file)
        {
            JsonDocument doc;

            // Serialize the empty JSON object to the file
            if (serializeJson(doc, file) == 0)
            {
                ESP_LOGI(SD_CARD_LOG_TAG, "Failed to write empty JSON object to the file");
            }
            else
            {
                ESP_LOGI(SD_CARD_LOG_TAG, "Empty JSON object written to file successfully");
            }

            file.close();
        }
        else
        {
            ESP_LOGI(SD_CARD_LOG_TAG, "Failed to open file for writing");
        }
    }
    else
    {
        ESP_LOGI(SD_CARD_LOG_TAG, "There's already JSON file of %s available!", filePath);
    }
}

/**
 * @brief Synchronizes RFID and Fingerprint data from SD card to a JsonObject.
 *
 * @return JsonObject The synchronized data from the RFID and Fingerprint files.
 */
JsonDocument SDCardModule::syncData()
{
    ESP_LOGI(SD_CARD_LOG_TAG, "Start sync data from ESP32 SD Card to Titan");

    JsonDocument document;
    JsonObject data = document.to<JsonObject>();
    const char *filePaths[] = {RFID_FILE_PATH, FINGERPRINT_FILE_PATH};

    // For future dev!
    // TODO : I'm lazy to do this in proper way, and I don't know when will it scale but for now this will do
    for (int i = 0; i < 2; i++)
    {
        const char *filePath = filePaths[i];
        ESP_LOGI(SD_CARD_LOG_TAG, "Reading file: %s", filePath);

        // Open the file from SD card
        File file = SD.open(filePath, FILE_READ);
        if (!file)
        {
            ESP_LOGI(SD_CARD_LOG_TAG, "Failed to open file: %s", filePath);
            continue;
        }

        // Use a smaller JsonDocument for each file read
        JsonDocument dataEachFile;
        DeserializationError error = deserializeJson(dataEachFile, file);
        if (error)
        {
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

bool SDCardModule::loadAllFingerprintIds(std::unordered_set<int> &idSet)
{
    File file = SD.open(FINGERPRINT_FILE_PATH, FILE_READ);
    if (!file)
    {
        ESP_LOGE(SD_CARD_LOG_TAG, "Failed to open fingerprint file for reading.");
        return false;
    }

    StaticJsonDocument<4096> document; // Adjust size as needed
    DeserializationError error = deserializeJson(document, file);
    file.close();

    if (error)
    {
        ESP_LOGE(SD_CARD_LOG_TAG, "Failed to deserialize JSON: %s", error.c_str());
        return false;
    }

    JsonArray users = document.as<JsonArray>();
    for (JsonObject user : users)
    {
        JsonArray fingerprintIds = user["key_access"].as<JsonArray>();
        for (int id : fingerprintIds)
        {
            idSet.insert(id);
        }
    }

    return true;
}
