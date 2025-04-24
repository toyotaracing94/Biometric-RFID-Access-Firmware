#define NFC_SERVICE_LOG_TAG "NFC_SERVICE"
#include "NFCService.h"
#include <esp_log.h>

NFCService::NFCService(AdafruitNFCSensor *nfcSensor, SDCardModule *sdCardModule, DoorRelay *doorRelay, BLEModule* bleModule, QueueHandle_t nfcQueueRequest, QueueHandle_t nfcQueueResponse) 
    : _nfcSensor(nfcSensor), _sdCardModule(sdCardModule), _doorRelay(doorRelay), _bleModule(bleModule), _nfcQueueRequest(nfcQueueRequest), _nfcQueueResponse(nfcQueueResponse){
    setup();
}

bool NFCService::setup() {
    ESP_LOGI(NFC_SERVICE_LOG_TAG, "NFC Service Creation");
    return true;
}

/**
 * @brief Add a new NFC card control and save its UID to server and the SD card.
 *
 * This function attempts to read the NFC card's UID and saves it to the server and SD card under the
 * specified username and visitor ID.
 *
 * @param username The username for NFC registration.
 * @return true if NFC UID is successfully saved to the SD card, false otherwise.
 */
bool NFCService::addNFC(const char *username) {
    ESP_LOGI(NFC_SERVICE_LOG_TAG, "Enrolling new NFC Access User! Username: %s", username);

    uint16_t timeout = 5000;
    ESP_LOGI(NFC_SERVICE_LOG_TAG, "Awaiting NFC Card Input! Timeout %d ms", timeout);
    sendbleNotification(READY_FOR_NFC_CARD_INPUT);
    
    char *uidCard = _nfcSensor->readNFCCard(timeout);
    if (uidCard == nullptr || uidCard[0] == '\0') {
        ESP_LOGW(NFC_SERVICE_LOG_TAG, "No NFC card detected for User: %s!", username);
        sendbleNotification(NFC_CARD_TIMEOUT);
        return false;
    }

    ESP_LOGI(NFC_SERVICE_LOG_TAG, "NFC card detected for User: %s with UID: %s", username, uidCard);
    
    // Pass the message into the queue message format
    NFCQueueRequest msg;
    snprintf(msg.username, sizeof(msg.username), "%s", username);
    snprintf(msg.uidCard, sizeof(msg.uidCard), "%s", uidCard);
    snprintf(msg.vehicleInformationNumber, sizeof(msg.vehicleInformationNumber), "%s", VIN);
    msg.state = ENROLL_RFID;
    msg.statusCode = START_REGISTERING_NFC_CARD_ACCESS;
    
    // Send back the information to the server first to get the visitor id pass first from server
    if (xQueueSend(_nfcQueueRequest, &msg, portMAX_DELAY) != pdPASS) {
        return handleError(FAILED_TO_SEND_NFC_TO_WIFI_QUEUE, username, "", "Failed to send NFC message to WiFi queue", false);
    }
    
    // Catch back the response from the queue with timeout of 5s
    // Will just mark for now that if there's no response from the wifi task, it probably failed
    // and I'll immediately return false
    NFCQueueResponse resp;
    if (xQueueReceive(_nfcQueueResponse, &resp, pdMS_TO_TICKS(5000)) != pdPASS) {
        return handleError(FAILED_TO_RECV_NFC_FROM_WIFI_QUEUE, username, "", "Failed to receive NFC message response from WiFi queue!", false);
    }

    // TODO: Make the request id to match with the response

    // Parse JSON response to extract visitor_id
    JsonDocument document;
    DeserializationError error = deserializeJson(document, resp.response);

    if (error) {
        return handleError(FAILED_TO_PARSE_RESPONSE, username, "", "Failed to parse the response!", false);
    }

    int statusCode = document["stat_code"].as<int>();
    if (statusCode != 200) {
        return handleError(FAILED_REQUEST_TO_SERVER, username, "", "Failed request to server!", false);
    }

    const char* visitorId = document["data"]["visitor_id"];
    if (visitorId == nullptr || strlen(visitorId) == 0) {
        return handleError(FAILED_GET_VISITORID_FROM_SERVER, username, "", "Failed to get Visitor ID from server!", false);
    }

    // This steps start saving to local ESP FS as already been confirmed on the server side
    ESP_LOGI(NFC_SERVICE_LOG_TAG, "Saving NFC Card Access with Visitor ID: %s", visitorId);
    bool saveNFCtoSDCard = _sdCardModule->saveNFCToSDCard(username, uidCard, visitorId);

    if (!saveNFCtoSDCard){
        // Delete back the visitorId that has been saved to the server
        // As failed to save data into SD Card
        ESP_LOGE(NFC_SERVICE_LOG_TAG, "Failed to save NFC UID %s to SD card for User: %s", uidCard, username);
        return handleError(FAILED_SAVE_NFC_ACCESS_TO_SD_CARD, username, visitorId, "Failed to save NFC UID to SD Card", true);
    }

    sendbleNotification(SUCCESS_REGISTERING_NFC_ACCESS);
    ESP_LOGI(NFC_SERVICE_LOG_TAG, "NFC UID %s successfully saved to SD card for User: %s", uidCard, username);
    return true;
}

/**
 * @brief Delete an NFC card from the SD card associated with the given username.
 *
 * This function attempts to delete the NFC card UID associated with a user from the SD card.
 * If the NFC card UID is empty or passed as "null", no deletion will occur.
 *
 * @param visitorId The Visitor ID that was associated with the NFC UID from the server to be deleted at SD Card
 * @return true if the NFC UID was successfully deleted from the SD card, false otherwise.  
 */
bool NFCService::deleteNFC(const char *visitorId) {
    ESP_LOGI(NFC_SERVICE_LOG_TAG, "Deleting NFC User! Visitor ID = %s", visitorId);

    // Delete first the data that on the server side
    // Pass the message into the queue message format
    NFCQueueRequest msg;
    snprintf(msg.visitorId, sizeof(msg.visitorId), "%s", visitorId);
    msg.state = DELETE_RFID;

    // Send back the information to the server first to delete them from server then in the esp
    if (xQueueSend(_nfcQueueRequest, &msg, portMAX_DELAY) != pdPASS) {
        return handleDeleteError(FAILED_TO_SEND_FINGERPRINT_TO_WIFI_QUEUE, visitorId, "Failed to send NFC message to WiFi queue!");
    }

    // Catch back the response from the queue with timeout of 5s
    // Will just mark for now that if there's now response from the wifi task, it probably failed 
    NFCQueueResponse resp;
    if (xQueueReceive(_nfcQueueResponse, &resp, pdMS_TO_TICKS(5000)) != pdPASS) {
        return handleDeleteError(FAILED_TO_RECV_FINGERPRINT_FROM_WIFI_QUEUE, visitorId, "Failed to receive NFC message response from WiFi queue");
    }
    ESP_LOGI(NFC_SERVICE_LOG_TAG, "Response: %s", resp.response);

    // If there is response, then the operation probably(?) yes lol success
    // and then we can proceed to delete the key access from the firmware side
    bool deleteNFCfromSDCard = _sdCardModule->deleteNFCFromSDCard(visitorId);

    if (!deleteNFCfromSDCard) {
        ESP_LOGI(NFC_SERVICE_LOG_TAG, "NFC card deleted successfully for Visitor ID: %s", visitorId);
        return handleDeleteError(FAILED_DELETE_NFC_ACCESS_TO_SD_CARD, visitorId, "Failed to delete NFC Card from SD Card");
        
    }
    // Prepare the data payload
    sendbleNotification(SUCCESS_DELETING_NFC_ACCESS);
    return true;
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
    char *uidCard = _nfcSensor->readNFCCard();
    if (uidCard == nullptr || uidCard[0] == '\0'){ return false; }
    else {
        if(_sdCardModule->isNFCIdRegistered(uidCard)){
            ESP_LOGI(NFC_SERVICE_LOG_TAG, "NFC Card Match with ID %s", uidCard);
            _doorRelay->toggleRelay();

            // Get the visitor Id of that UID Card
            std::string *visitorId = _sdCardModule->getVisitorIdByNFC(uidCard);

            // Send the access history without waiting the response
            NFCQueueRequest msg;
            msg.state = AUTEHNTICATE_RFID;
            snprintf(msg.visitorId, sizeof(msg.visitorId), "%s", visitorId->c_str());
            snprintf(msg.uidCard, sizeof(msg.uidCard), "%s", uidCard);

            if (xQueueSend(_nfcQueueRequest, &msg, portMAX_DELAY) != pdPASS) {
                ESP_LOGE(NFC_SERVICE_LOG_TAG, "Failed to send NFC message to WiFi queue!");
            }
            
            return true;
        }

        ESP_LOGI(NFC_SERVICE_LOG_TAG, "NFC Card ID %s is detected but not stored in our data system!", uidCard);
        return false;
    }
}

/**
 * @brief Broadcasts a BLE notification about an NFC service event.
 * 
 *
 * @param status     The status code or label to indicate success or error (e.g., "OK", "ERR").
 * @param username   The name of the visitor or user.
 * @param visitorId  The unique identifier of the visitor.
 * @param message    A descriptive message to include in the BLE notification.
 * @param type       The type of authentication used (e.g., "RFID", "FINGERPRINT").
 *
 * @deprecated This function is will soon deprecated and remove. Use the new 'sendbleNotification' with int parameter instead.  
 */
[[deprecated("This function is will soon deprecated and remove. Use the new 'sendbleNotification' with int parameter instead.")]]
void NFCService::sendbleNotification(const char *status, const char *username, const char *visitorId, const char *message, const char *type) {
    JsonDocument doc;
    doc["data"]["name"] = username;
    doc["data"]["visitor_id"] = visitorId;
    doc["data"]["type"] = type;
    _bleModule->sendReport(status, doc.as<JsonObject>(), message);
}

/**
 * @brief Sends a BLE notification with NFC-related status and message.
 *
 * This function constructs a JSON object containing status code of the result action,
 * then sends it using the BLE module with the specified status.
 *
 * @param status int Status code
 */
void NFCService::sendbleNotification(int statusCode){
    ESP_LOGI(NFC_SERVICE_LOG_TAG, "Sending Fingerprint Service Action Result to BLE Notification");
    _bleModule->sendReport(statusCode);
}


/**
 * @brief Handles NFC operation errors by sending BLE notifications and optional cleanup.
 *
 * Logs an error, sends a BLE error notification, and optionally attempts to clean up
 * the server-side data (e.g., removing the registered NFC Card from server if the local save fails).
 *
 * @param statusCode The status code related to the error from the `ErrorCode` enum
 * @param username Name of the user associated with the operation.
 * @param visitorId Visitor ID to be used for cleanup (can be nullptr if not applicable).
 * @param message Error message to log and send in the notification.
 * @param cleanup If true and visitorId is provided, will send a REMOVE_FP request to the server.
 * @return Always returns false
 */
bool NFCService::handleError(int statusCode, const char* username, const char* visitorId, const char* message, bool cleanup) {
    sendbleNotification(statusCode);
    ESP_LOGE(NFC_SERVICE_LOG_TAG, "%s", message);

    if (cleanup && visitorId) {
        NFCQueueRequest msg;
        msg.state = DELETE_RFID;
        snprintf(msg.visitorId, sizeof(msg.visitorId), "%s", visitorId);
        snprintf(msg.username, sizeof(msg.username), "%s", username);
        snprintf(msg.vehicleInformationNumber, sizeof(msg.vehicleInformationNumber), "%s", VIN);

        if (xQueueSend(_nfcQueueRequest, &msg, portMAX_DELAY) != pdPASS) {
            ESP_LOGE(NFC_SERVICE_LOG_TAG, "Failed to send cleanup request for visitor ID: %s", visitorId);
        }

        NFCQueueResponse resp;
        if (xQueueReceive(_nfcQueueResponse, &resp, pdMS_TO_TICKS(5000)) != pdPASS) {
            ESP_LOGE(NFC_SERVICE_LOG_TAG, "No response received for cleanup request.");
        } else {
            ESP_LOGI(NFC_SERVICE_LOG_TAG, "Cleanup response: %s", resp.response);
        }
    }

    return false;
}

/**
 * @brief Handles NFC operation errors of deletion operation by sending BLE notifications.
 *
 * Logs an error and sends a BLE error notification. No need for cleanup as you know
 * This is for handle of deletion error, what are we going to cleanup? We already are!
 *
 * @param statusCode The status code related to the error from the `ErrorCode` enum
 * @param visitorId Visitor ID
 * @param message Error message to log and send in the notification.
 * @return Always returns false
 */
bool NFCService::handleDeleteError(int statusCode, const char* visitorId, const char* message) {
    sendbleNotification(statusCode);
    ESP_LOGE(NFC_SERVICE_LOG_TAG, "%s", message);
    return false;
}