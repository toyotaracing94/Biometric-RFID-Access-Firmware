#define NFC_SERVICE_LOG_TAG "NFC_SERVICE"
#include "NFCService.h"
#include <esp_log.h>
#include <communication/wifi/service/WiFiModule.h>
#include "../src/config/Config.h"

NFCService::NFCService(AdafruitNFCSensor *nfcSensor, SDCardModule *sdCardModule, DoorRelay *doorRelay, BLEModule *bleModule, WiFiModule *wifiModule)
    : _nfcSensor(nfcSensor), _sdCardModule(sdCardModule), _doorRelay(doorRelay), _bleModule(bleModule), _wifiModule(wifiModule)
{
    setup();
}

bool NFCService::setup()
{
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
bool NFCService::addNFC(const char *username)
{
    ESP_LOGI(NFC_SERVICE_LOG_TAG, "Enrolling new NFC Access User! Username: %s", username);

    uint16_t timeout = 10000;
    ESP_LOGI(NFC_SERVICE_LOG_TAG, "Awaiting NFC Card Input! Timeout %d ms", timeout);
    sendbleNotification("ENROLL", username, "0:0:0:0", "Please place your NFC Card", "RFID");

    char *uidCard = _nfcSensor->readNFCCard(timeout);

    if (uidCard == nullptr || uidCard[0] == '\0')
    {
        ESP_LOGW(NFC_SERVICE_LOG_TAG, "No NFC card detected for User: %s!", username);
        return false;
    }
    else
    {
        ESP_LOGI(NFC_SERVICE_LOG_TAG, "NFC card detected for User: %s with UID: %s", username, uidCard);
        std::string url = "http://203.100.57.59:3000/api/v1/user-vehicle/visitor";

        StaticJsonDocument<256> payloadDoc;
        payloadDoc["vin"] = VIN;
        payloadDoc["visitor_name"] = username;
        payloadDoc["type"] = "RFID";
        std::string payload;
        serializeJson(payloadDoc, payload);

        std::string response = _wifiModule->sendPostRequest(url, payload);
        ESP_LOGI(NFC_SERVICE_LOG_TAG, "Response: %s", response.c_str());

        // Parse JSON response to extract visitor_id
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, response.c_str());

        if (error)
        {
            ESP_LOGE(NFC_SERVICE_LOG_TAG, "Failed to parse response: %s", error.c_str());
            return false;
        }

        int statusCode = doc["stat_code"].as<int>(); // Assuming stat_code is an integer field
        if (statusCode != 200)
        {
            ESP_LOGE(NFC_SERVICE_LOG_TAG, "Request failed with status code: %d", statusCode);
            sendbleNotification("ERR", username, "", "RFID", "Failed to send request to server!");
            return false;
        }
        std::string visitorId = doc["data"]["visitor_id"].as<std::string>();

        if (visitorId.empty())
        {
            ESP_LOGE(NFC_SERVICE_LOG_TAG, "Failed to extract visitor_id from response");
            return false;
        }

        ESP_LOGI(NFC_SERVICE_LOG_TAG, "Visitor ID: %s", visitorId.c_str());

        bool saveNFCtoSDCard = _sdCardModule->saveNFCToSDCard(username, uidCard, visitorId.c_str());

        if (saveNFCtoSDCard)
        {
            // Prepare the data payload

            char status[5] = "OK";
            char message[50] = "NFC Card registered successfully!";
            sendbleNotification(status, username, visitorId.c_str(), message, "RFID");
            ESP_LOGI(NFC_SERVICE_LOG_TAG, "NFC UID %s successfully saved to SD card for User: %s", uidCard, username);
        }
        else
        {
            // Prepare the data payload
            char status[5] = "ERR";
            char message[50] = "Failed to register NFC card!";
            std::string url = "http://203.100.57.59:3000/api/v1/user-vehicle/visitor/" + std::string(visitorId);
            std::string response = _wifiModule->sendDeleteRequest(url);
            ESP_LOGI(NFC_SERVICE_LOG_TAG, "Response: %s", response.c_str());
            sendbleNotification(status, username, visitorId.c_str(), message, "RFID");
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
bool NFCService::deleteNFC(const char *visitorId)
{
    ESP_LOGI(NFC_SERVICE_LOG_TAG, "Deleting NFC User! Visitor ID = %s", visitorId);

    bool deleteNFCfromSDCard = _sdCardModule->deleteNFCFromSDCard(visitorId);

    if (deleteNFCfromSDCard)
    {
        // Prepare the data payload
        char status[5] = "OK";
        char message[50] = "NFC Card deleted successfully!";
        std::string url = "http://203.100.57.59:3000/api/v1/user-vehicle/visitor/" + std::string(visitorId);
        std::string response = _wifiModule->sendDeleteRequest(url);
        ESP_LOGI(NFC_SERVICE_LOG_TAG, "Response: %s", response.c_str());
        sendbleNotification(status, "", visitorId, message, "RFID");
        ESP_LOGI(NFC_SERVICE_LOG_TAG, "NFC card deleted successfully for Visitor ID: %s", visitorId);
    }
    else
    {
        // Prepare the data payload
        char status[5] = "ERR";
        char message[50] = "Failed to delete NFC card!";
        sendbleNotification(status, "", visitorId, message, "RFID");
        ESP_LOGE(NFC_SERVICE_LOG_TAG, "Failed to delete NFC card for Visitor ID: %s", visitorId);
    }
    return deleteNFCfromSDCard;
}

/**
 * @brief Authenticate access using an NFC card.
 *
 * This function reads the NFC card and checks if the UID is registered in the system.
 *
 * @return true if the NFC card UID matches a registered entry and access is granted;
 *         false if no card is detected or the UID is not registered.
 */
bool NFCService::authenticateAccessNFC()
{
    char *uidCard = _nfcSensor->readNFCCard();
    if (uidCard == nullptr || uidCard[0] == '\0')
    {
        return false;
    }
    else
    {
        std::string *visitorId = _sdCardModule->getVisitorIdByNFC(uidCard);
        if (visitorId != nullptr)
        {
            ESP_LOGI(NFC_SERVICE_LOG_TAG, "NFC Card Match with ID %s", uidCard);
            _doorRelay->toggleRelay();
            std::string url = "http://203.100.57.59:3000/api/v1/user-vehicle/visitor/activity";
            std::string payload = R"({"visitor_id": ")" + *visitorId + R"("})";

            std::string response = _wifiModule->sendPostRequest(url, payload);
            ESP_LOGI(NFC_SERVICE_LOG_TAG, "Response: %s", response.c_str());
            return true;
        }
        ESP_LOGI(NFC_SERVICE_LOG_TAG, "NFC Card ID %s is detected but not stored in our data system!", uidCard);
        return false;
    }
}

void NFCService::sendbleNotification(const char *status, const char *username, const char *visitorId, const char *message, const char *type)
{
    JsonDocument doc;
    doc["data"]["name"] = username;
    doc["data"]["visitor_id"] = visitorId;
    doc["data"]["type"] = type;
    _bleModule->sendReport(status, doc.as<JsonObject>(), message);
}
