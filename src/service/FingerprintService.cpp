#define FINGERPRINT_SERVICE_LOG_TAG "FINGERPRINT_SERVICE"
#include "FingerprintService.h"
#include <esp_log.h>

FingerprintService::FingerprintService(FingerprintSensor *fingerprintSensor, SDCardModule *sdCardModule, DoorRelay *doorRelay, BLEModule *bleModule, QueueHandle_t fingerprintQueueRequest, QueueHandle_t fingerprintQueueResponse) 
    : _fingerprintSensor(fingerprintSensor), _sdCardModule(sdCardModule), _doorRelay(doorRelay), _bleModule(bleModule), _fingerprintQueueRequest(fingerprintQueueRequest), _fingerprintQueueResponse(fingerprintQueueResponse){
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
 * @return
 *      - true if the fingerprint model was successfully added and Fingerprint ID saved to the SD card; false otherwise.  
 */
bool FingerprintService::addFingerprint(const char *username) {
    uint8_t fingerprintId = generateFingerprintId();
    ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Enrolling new Fingerprint User! Username %s FingerprintID %d", username, fingerprintId);

    // Getting the visitorId first from the server
    // Pass the message into the queue message format
    FingerprintQueueRequest msg;
    msg.state = ENROLL_FP;
    msg.fingerprintId = fingerprintId;
    snprintf(msg.username, sizeof(msg.username), "%s", username);
    snprintf(msg.vehicleInformationNumber, sizeof(msg.vehicleInformationNumber), "%s", VIN);
    msg.statusCode = START_REGISTERING_FINGERPRINT_ACCESS;

    // Send back the information to the server first to get the visitor id pass first from server
    if (xQueueSend(_fingerprintQueueRequest, &msg, portMAX_DELAY) != pdPASS) {
        return handleError(FAILED_TO_SEND_FINGERPRINT_TO_WIFI_QUEUE, username, "", "Failed to send Fingerprint message to WiFi queue!", false);
    }

    // Catch back the response from the queue with timeout of 5s
    // Will just mark for now that if there's no response from the wifi task, it probably failed
    // and I'll immediately return false
    FingerprintQueueResponse resp;
    if (xQueueReceive(_fingerprintQueueResponse, &resp, pdMS_TO_TICKS(5000)) != pdPASS) {
        return handleError(FAILED_TO_RECV_FINGERPRINT_FROM_WIFI_QUEUE, username, "", "Failed to receive Fingerprint message response from WiFi queue!", false);
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

    // Start registering the fingerprint / turn on the protocol for registering fingerprint on sensor side
    // as the data already been confirmed has been saved into the server
    sendbleNotification(START_REGISTERING_FINGERPRINT_ACCESS);

    if (!_fingerprintSensor->addFingerprintModel(fingerprintId, std::bind(&FingerprintService::addFingerprintCallback, this, std::placeholders::_1))) {
        return handleError(FAILED_TO_ADD_FINGERPRINT_MODEL, username, "", "Failed to add Fingerprint Model!", true);
    }

    ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Fingerprint model added successfully for FingerprintID: %d Under Visitor ID %s. Saving to SD card...", fingerprintId, visitorId);
    // Save the fingerprint data to SD card
    if (!_sdCardModule->saveFingerprintToSDCard(username, fingerprintId, visitorId)){
        // If the save fingerprint to SD Card failed
        // Delete back the visitorId that has been saved to the server
        return handleError(FAILED_SAVE_FINGERPRINT_ACCESS_TO_SD_CARD, username, visitorId, "Failed to register Fingerprint to SD Card!", true);
    }

    ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Fingerprint saved to SD card successfully for User: %s, VisitorID: %s, FingerprintID: %d", username, visitorId, fingerprintId);
    sendbleNotification(SUCCESS_REGISTERING_FINGERPRINT_ACCESS);
    return true;
}

/**
 * @brief Delete a fingerprint access control
 *
 * This function attempts to delete a fingerprint model from the fingerprint sensor and
 * also removes the associated fingerprint data from the SD card.
 *
 * @param visitorId The Visitor ID that was associated with the fingerprint ID from the server to be deleted at SD Card
 * @return true if the fingerprint was successfully deleted from both the sensor and the SD card;
 *         false otherwise.
 */
bool FingerprintService::deleteFingerprint(const char *visitorId) {
    ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Deleting Fingerprint User! Visitor ID = %s", visitorId);

    // Delete first the data that on the server side
    // Pass the message into the queue message format
    FingerprintQueueRequest msg;
    snprintf(msg.visitorId, sizeof(msg.visitorId), "%s", visitorId);
    msg.state = DELETE_FP;

    // Send back the information to the server first to get the visitor id pass first from server
    if (xQueueSend(_fingerprintQueueRequest, &msg, portMAX_DELAY) != pdPASS) {
        return handleDeleteError(FAILED_TO_SEND_FINGERPRINT_TO_WIFI_QUEUE, visitorId, "Failed to send Fingerprint message to WiFi queue!");
    }

    // Also Catch back the response from the queue with timeout of 5s
    // Will just mark for now that if there's no response from the wifi task, it probably failed
    // and I'll immediately return false
    FingerprintQueueResponse resp;
    if (xQueueReceive(_fingerprintQueueResponse, &resp, pdMS_TO_TICKS(5000)) != pdPASS) {
        return handleDeleteError(FAILED_TO_RECV_FINGERPRINT_FROM_WIFI_QUEUE, visitorId, "Failed to receive Fingerprint message response from WiFi queue!");
    }
    ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Response: %s", resp.response);

    // Get the fingerprintId from the SD Card
    int fingerprintId = _sdCardModule->getFingerprintIdByVisitorId(visitorId);
    if (fingerprintId <= 0) {
        return handleDeleteError(FAILED_TO_RETRIEVE_VISITORID_FROM_SDCARD, visitorId, "Failed to retrieve user data for Visitor ID!");
    }

    // If success there is visitorId associated with fingerprint, then delete them from the sensor and from the SD Card
    ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Deleting Fingerprint ID %d from sensor! ", fingerprintId);
    bool deleteFingerprintResultSensor = _fingerprintSensor->deleteFingerprintModel(fingerprintId);

    if (deleteFingerprintResultSensor) {
        ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Fingerprint model deleted successfully from sensor for FingerprintID: %d", fingerprintId);
        bool deleteFingerprintSDCard = _sdCardModule->deleteFingerprintFromSDCard(visitorId);

        if (!deleteFingerprintSDCard) {
            return handleDeleteError(FAILED_DELETE_FINGERPRINT_ACCESS_FROM_SD_CARD, visitorId, "Failed to delete Fingerprint from SD card!");
        }

        // Prepare the data payload
        ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Fingerprint deleted from SD card successfully for FingerprintID: %d", fingerprintId);
        sendbleNotification(SUCCESS_DELETING_FINGERPRINT_ACCESS);
        return true;

    } else {
        return handleDeleteError(FAILED_TO_DELETE_FINGERPRINT_MODEL, visitorId, "Failed to delete fingerprint model from sensor for FingerprintID: %d");
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
    int isRegsiteredModel = _fingerprintSensor->getFingerprintIdModel();
    if(isRegsiteredModel > 0){
        if(_sdCardModule->isFingerprintIdRegistered(isRegsiteredModel)){
            ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Fingerprint Match with ID %d", isRegsiteredModel);
            _doorRelay->toggleRelay();

            // Get the visitor Id of that Fingerprint ID
            std::string *visitorId = _sdCardModule->getVisitorIdByFingerprintId(isRegsiteredModel);

            // Send the access history without waiting the response
            FingerprintQueueRequest msg;
            msg.state = AUTHENTICATE_FP;
            msg.fingerprintId = isRegsiteredModel;
            snprintf(msg.visitorId, sizeof(msg.visitorId), "%s", visitorId->c_str());

            if (xQueueSend(_fingerprintQueueRequest, &msg, portMAX_DELAY) != pdPASS) {
                ESP_LOGE(FINGERPRINT_SERVICE_LOG_TAG, "Failed to send NFC message to WiFi queue!");
            }

            return true;
        }
        // TODO : Perhaps implement callback that tell the FingerprintModel is save correctly, but the data is not save into the Microcontroller System
        ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Fingerprint Model ID %d is Registered on Sensor, but not appear in stored data. Cannot open the Door Lock", isRegsiteredModel);
        return false;
    }
    else{
        ESP_LOGD(FINGERPRINT_SERVICE_LOG_TAG, "Fingerprint does not match the stored data. Access denied");
        return false;
    }
}

/**
 * @brief Generate Fingerprint ID to be used for associated with the Fingerprint model that was saved into the senosr
 * 
 * This function will generate an int of 1-127 for fingerprintId
 * 
 * @return uint8_t Fingerprint ID
 */
uint8_t FingerprintService::generateFingerprintId(){
    uint8_t fingerprintId;
    while (true){
        fingerprintId = random(1, 100);
        // Check if the generated fingerprint ID exists on the SD card
        if (!_sdCardModule->isFingerprintIdRegistered(fingerprintId)){
            // If the ID doesn't exist on the SD card, it's valid
            ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Generated valid fingerprint ID: %d", fingerprintId);
            break;
        }
        else{
            // If the ID exists on the SD card, try again
            ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Fingerprint ID %d already exists, generating a new ID...", fingerprintId);
        }
    }
    return fingerprintId;
}

/**
 * @brief Sends a BLE notification with fingerprint-related status and message.
 *
 * This function constructs a JSON object containing user and fingerprint information,
 * then sends it using the BLE module with the specified status and message.
 *
 * @param status Status code string (e.g., "OK", "ERR", "ENROLL").
 * @param username Name of the user associated with the fingerprint operation.
 * @param visitorId Unique visitor ID obtained from the server.
 * @param type Type of the operation or context (usually "Fingerprint").
 * @param message Descriptive message to include in the notification.
 * 
 * @deprecated This function is will soon deprecated and remove. Use the new 'sendbleNotification' with int parameter instead.
 */
[[deprecated("This function is will soon deprecated and remove. Use the new 'sendbleNotification' with int parameter instead.")]]
void FingerprintService::sendbleNotification(const char *status, const char *username, const char *visitorId, const char *type, const char *message){
    ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Sending Fingerprint Service Action Result to BLE Notification");
    JsonDocument doc;
    doc["data"]["name"] = username;
    doc["data"]["visitor_id"] = visitorId;
    doc["data"]["type"] = type;
    _bleModule->sendReport(status, doc.as<JsonObject>(), message);
}

/**
 * @brief Sends a BLE notification with fingerprint-related status and message.
 *
 * This function constructs a JSON object containing status code of the result action,
 * then sends it using the BLE module with the specified status.
 *
 * @param status int Status code
 */
void FingerprintService::sendbleNotification(int statusCode){
    ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Sending Fingerprint Service Action Result to BLE Notification");
    _bleModule->sendReport(statusCode);
}

/**
 * @brief Handles fingerprint operation errors by sending BLE notifications and optional cleanup.
 *
 * Logs an error, sends a BLE error notification, and optionally attempts to clean up
 * the server-side data (e.g., removing the registered fingerprint if the local save fails).
 *
 * @param statusCode The status code related to the error from the `ErrorCode` enum
 * @param username Name of the user associated with the operation.
 * @param visitorId Visitor ID to be used for cleanup (can be nullptr if not applicable).
 * @param message Error message to log and send in the notification.
 * @param cleanup If true and visitorId is provided, will send a REMOVE_FP request to the server.
 * @return Always returns false
 */
bool FingerprintService::handleError(int statusCode, const char* username, const char* visitorId, const char* message, bool cleanup) {
    ESP_LOGE(FINGERPRINT_SERVICE_LOG_TAG, "%s", message);
    sendbleNotification(statusCode);

    if (cleanup && visitorId) {
        FingerprintQueueRequest msg;
        msg.state = DELETE_FP;
        snprintf(msg.visitorId, sizeof(msg.visitorId), "%s", visitorId);
        snprintf(msg.username, sizeof(msg.username), "%s", username);
        snprintf(msg.vehicleInformationNumber, sizeof(msg.vehicleInformationNumber), "%s", VIN);

        if (xQueueSend(_fingerprintQueueRequest, &msg, portMAX_DELAY) != pdPASS) {
            ESP_LOGE(FINGERPRINT_SERVICE_LOG_TAG, "Failed to send cleanup request for visitor ID: %s", visitorId);
        }

        FingerprintQueueResponse resp;
        if (xQueueReceive(_fingerprintQueueResponse, &resp, pdMS_TO_TICKS(5000)) != pdPASS) {
            ESP_LOGE(FINGERPRINT_SERVICE_LOG_TAG, "No response received for cleanup request.");
        } else {
            ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Cleanup response: %s", resp.response);
        }
    }
    return false;
}

/**
 * @brief Handles fingerprint operation errors of deletion operation by sending BLE notifications.
 *
 * Logs an error and sends a BLE error notification. No need for cleanup as you know
 * This is for handle of deletion error, what are we going to cleanup? We already are!
 *
 * @param statusCode The status code related to the error from the `ErrorCode` enum
 * @param visitorId Visitor ID
 * @param message Error message to log and send in the notification.
 * @return Always returns false  
 */
bool FingerprintService::handleDeleteError(int statusCode, const char* visitorId, const char* message) {
    ESP_LOGE(FINGERPRINT_SERVICE_LOG_TAG, "Error Code: %d, %s", statusCode, message);
    sendbleNotification(statusCode);
    return false;
}

/**
 * @brief A Callback function to handle the notification of the current fingerprint status when registering new fingerprint model
 * that where will send the status code to external device over BLE network
 * 
 * @param statusCode The status code related to the state of the fingerprint, Success or failed operation  
 */
void FingerprintService::addFingerprintCallback(int statusCode){
    ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Start sending callback Status Code! Status Code %d", statusCode);
    sendbleNotification(statusCode);
}
