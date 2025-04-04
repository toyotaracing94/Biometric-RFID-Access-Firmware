#define FINGERPRINT_SERVICE_LOG_TAG "FINGERPRINT_SERVICE"
#include "FingerprintService.h"
#include <esp_log.h>
#include "../src/config/Config.h"
#include <unordered_set>
FingerprintService::FingerprintService(FingerprintSensor *fingerprintSensor, SDCardModule *sdCardModule, DoorRelay *doorRelay, BLEModule *bleModule, WiFiModule *wifiModule)
    : _fingerprintSensor(fingerprintSensor), _sdCardModule(sdCardModule), _doorRelay(doorRelay), _bleModule(bleModule), _wifiModule(wifiModule)
{
    setup();
}

bool FingerprintService::setup()
{
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
bool FingerprintService::addFingerprint(const char *username)
{

    uint8_t fingerprintId = generateFingerprintId();
    ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Enrolling new Fingerprint User! Username %s FingerprintID %d", username, fingerprintId);
    std::string url = "http://203.100.57.59:3000/api/v1/user-vehicle/visitor";

    StaticJsonDocument<256> payloadDoc;
    payloadDoc["vin"] = VIN;
    payloadDoc["visitor_name"] = username;
    payloadDoc["type"] = "Fingerprint";
    std::string payload;
    serializeJson(payloadDoc, payload);

    std::string response = _wifiModule->sendPostRequest(url, payload);
    if (response.empty())
    {
        ESP_LOGE(FINGERPRINT_SERVICE_LOG_TAG, "Failed to get response");
        return false;
    }
    ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Response: %s", response.c_str());

    // Parse JSON response to extract visitor_id
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, response.c_str());

    if (error)
    {
        ESP_LOGE(FINGERPRINT_SERVICE_LOG_TAG, "Failed to parse response: %s", error.c_str());
        return false;
    }

    int statusCode = doc["stat_code"].as<int>(); // Assuming stat_code is an integer field

    if (statusCode != 200)
    {
        ESP_LOGE(FINGERPRINT_SERVICE_LOG_TAG, "Request failed with status code: %d", statusCode);
        _fingerprintSensor->deleteFingerprintModel(fingerprintId);
        sendbleNotification("ERR", username, "", "Fingerprint", "Failed to send request to server!");

        return false;
    }

    // Extract visitor_id from the response
    std::string visitorId = doc["data"]["visitor_id"].as<std::string>();
    if (visitorId.empty())
    {
        ESP_LOGE(FINGERPRINT_SERVICE_LOG_TAG, "Visitor ID not found in response");
        return false;
    }
    sendbleNotification("ENROLL", username, "", "Fingerprint", "Please place your fingerprint to enroll!");
    if (!_fingerprintSensor->addFingerprintModel(fingerprintId))
    {
        ESP_LOGE(FINGERPRINT_SERVICE_LOG_TAG, "Failed to add fingerprint model for FingerprintID: %d", fingerprintId);
        sendbleNotification("ERR", username, "", "Fingerprint", "Failed to add Fingerprint Model!");
        return false;
    }

    sendbleNotification("VERIFY", username, "", "Fingerprint", "Please place your fingerprint to verify!");
    ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Please place your fingerprint to verify!");

    if (!_fingerprintSensor->verifyFingerprint(fingerprintId))
    {
        ESP_LOGE(FINGERPRINT_SERVICE_LOG_TAG, "Failed to verify fingerprint for FingerprintID: %d", fingerprintId);
        std::string url = "http://203.100.57.59:3000/api/v1/user-vehicle/visitor/" + std::string(visitorId);
        std::string response = _wifiModule->sendDeleteRequest(url);
        ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Response: %s", response.c_str());
        sendbleNotification("ERR", username, "", "Fingerprint", "Fingerprint verification failed!");
        return false;
    }

    ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Fingerprint model added successfully for FingerprintID: %d. Saving to SD card...", fingerprintId);

    ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Visitor ID: %s", visitorId.c_str());

    // Save the fingerprint data to SD card
    if (!_sdCardModule->saveFingerprintToSDCard(username, fingerprintId, visitorId.c_str()))
    {
        ESP_LOGE(FINGERPRINT_SERVICE_LOG_TAG, "Failed to save fingerprint to SD card for User: %s, FingerprintID: %d", username, fingerprintId);
        _fingerprintSensor->deleteFingerprintModel(fingerprintId);
        std::string url = "http://203.100.57.59:3000/api/v1/user-vehicle/visitor/" + std::string(visitorId);
        std::string response = _wifiModule->sendDeleteRequest(url);
        ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Response: %s", response.c_str());
        sendbleNotification("ERR", username, visitorId.c_str(), "Fingerprint", "Failed to save fingerprint to SD card!");
        return false;
    }

    sendbleNotification("OK", username, visitorId.c_str(), "Fingerprint", "Fingerprint registered successfully!");
    ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Fingerprint saved to SD card successfully for User: %s, FingerprintID: %d", username, fingerprintId);

    return true;
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
bool FingerprintService::deleteFingerprint(const char *visitorId)
{
    ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Deleting Fingerprint User! Visitor ID = %s", visitorId);
    JsonObject userData;
    if (!_sdCardModule->getFingerprintModelByVisitorId(visitorId, userData))
    {
        ESP_LOGE(FINGERPRINT_SERVICE_LOG_TAG, "Failed to retrieve user data for Visitor ID: %s", visitorId);
        return false;
    }

    const char *username = nullptr; // Initialize username to nullptr
    if (userData.containsKey("username") && userData["username"].is<const char *>())
    {
        username = userData["username"].as<const char *>();
    }
    else
    {
        ESP_LOGW(FINGERPRINT_SERVICE_LOG_TAG, "Username not found or is not a string for Visitor ID: %s", visitorId);
        // You might want to handle this case differently, such as using a default username.
        // For now, we'll proceed with username being null.
    }

    JsonArray fingerprintIds = userData["key_access"].as<JsonArray>();

    for (int i = 0; i < fingerprintIds.size(); i++)
    {
        uint8_t id = fingerprintIds[i];
        bool deleteFingerprintResultSensor = _fingerprintSensor->deleteFingerprintModel(id);
        if (!deleteFingerprintResultSensor)
        {
            ESP_LOGE(FINGERPRINT_SERVICE_LOG_TAG, "Failed to delete fingerprint model for FingerprintID: %d", id);
            sendbleNotification("ERR", username, visitorId, "Fingerprint", "Failed to delete Fingerprint Model!");
            return false;
        }
    }
    ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Fingerprint model deleted successfully for Visitor Id: %s", visitorId);

    bool deleteFingerprintSDCard = _sdCardModule->deleteFingerprintFromSDCard(visitorId);

    if (deleteFingerprintSDCard)
    {
        // Prepare the data payload

        char status[5] = "OK";
        char message[50] = "Fingerprint deleted successfully!";
        std::string url = "http://203.100.57.59:3000/api/v1/user-vehicle/visitor/" + std::string(visitorId);
        std::string response = _wifiModule->sendDeleteRequest(url);
        ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Response: %s", response.c_str());
        sendbleNotification(status, username, visitorId, "Fingerprint", message);
        ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Fingerprint deleted from SD card successfully for VisitorID: %s", visitorId);
    }
    else
    {
        // Prepare the data payload
        char status[5] = "ERR";
        char message[50] = "Failed to delete Fingerprint!";
        sendbleNotification(status, username, visitorId, "Fingerprint", message);
        ESP_LOGE(FINGERPRINT_SERVICE_LOG_TAG, "Failed to delete fingerprint from SD card for User: %s, FingerprintID: %s", username, visitorId);
    }
    return deleteFingerprintSDCard;
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
bool FingerprintService::authenticateAccessFingerprint()
{
    int isRegsiteredModel = _fingerprintSensor->getFingerprintIdModel();
    if (isRegsiteredModel > 0)
    {
        std::string *visitorId = _sdCardModule->getFingerprintModelById(isRegsiteredModel);

        if (visitorId != nullptr)
        {
            ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Fingerprint Match with ID %d", isRegsiteredModel);
            _doorRelay->toggleRelay();

            ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "VISITOR ID : %s", visitorId->c_str());
            std::string url = "http://203.100.57.59:3000/api/v1/user-vehicle/visitor/activity";
            std::string payload = R"({"visitor_id": ")" + *visitorId + R"("})";
            ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "PAYLOAD : %s", payload.c_str());

            std::string response = _wifiModule->sendPostRequest(url, payload);
            ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Response: %s", response.c_str());
            return true;
        }

        // TODO : Perhaps implement callback that tell the FingerprintModel is save correctly, but the data is not save into the Microcontroller System
        ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Fingerprint Model ID %d is Registered on Sensor, but not appear in stored data. Cannot open the Door Lock", isRegsiteredModel);
        return false;
    }
    else
    {
        ESP_LOGD(FINGERPRINT_SERVICE_LOG_TAG, "Fingerprint does not match the stored data. Access denied");
        return false;
    }
}
uint8_t FingerprintService::generateFingerprintId()
{
    uint8_t fingerprintId;
    while (true)
    {
        fingerprintId = random(1, 100);
        // Check if the generated fingerprint ID exists on the SD card
        if (!_sdCardModule->isFingerprintIdRegistered(fingerprintId))
        {
            // If the ID doesn't exist on the SD card, it's valid
            ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Generated valid fingerprint ID: %d", fingerprintId);
            break;
        }
        else
        {
            // If the ID exists on the SD card, try again
            ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Fingerprint ID %d already exists, generating a new ID...", fingerprintId);
        }
    }
    return fingerprintId;
}

void FingerprintService::sendbleNotification(const char *status, const char *username, const char *visitorId, const char *type, const char *message)
{
    JsonDocument doc;
    doc["data"]["name"] = username;
    doc["data"]["visitor_id"] = visitorId;
    doc["data"]["type"] = type;
    _bleModule->sendReport(status, doc.as<JsonObject>(), message);
}