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
 * @param visitorId The Visitor ID that represent the user of the key access
 * @param keyAccessId The Key Access ID that represent the fingerprint key access in the server
 * @return
 *      - true if the fingerprint model was successfully added and Fingerprint ID saved to the SD card; false otherwise.  
 */
bool FingerprintService::addFingerprint(const char *username, const char *visitorId, const char *keyAccessId) {
    uint8_t fingerprintId = generateFingerprintId();
    ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Enrolling new Fingerprint User! Username %s, ID %d, VisitorId %s, KeyAccessId %s", username, fingerprintId, visitorId, keyAccessId);

    // Start registering the fingerprint / turn on the protocol for registering fingerprint on sensor side
    sendbleNotification(START_REGISTERING_FINGERPRINT_ACCESS);

    if (!_fingerprintSensor->addFingerprintModel(fingerprintId, std::bind(&FingerprintService::addFingerprintCallback, this, std::placeholders::_1))) {
        return handleError(FAILED_TO_ADD_FINGERPRINT_MODEL, username, visitorId, "Failed to add Fingerprint Model!", false);
    }

    ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Fingerprint model added successfully for FingerprintID: %d Under Visitor ID %s Key Access ID %s. Saving to SD card...", fingerprintId, visitorId, keyAccessId);
    // Save the fingerprint data to SD card
    if (!_sdCardModule->saveFingerprintToSDCard(username, fingerprintId, visitorId, keyAccessId)){
        // If the save fingerprint to SD Card failed
        // Delete the data from the sensor
        _fingerprintSensor->deleteFingerprintModel(fingerprintId);

        // Delete back the visitorId that has been saved to the server
        return handleError(FAILED_SAVE_FINGERPRINT_ACCESS_TO_SD_CARD, username, visitorId, "Failed to register Fingerprint to SD Card!", false);
    }

    ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Fingerprint saved to SD card successfully for User: %s, FingerprintID: %d, VisitorID: %s, KeyAccessID : %s", username, fingerprintId, visitorId, keyAccessId);
    sendbleNotification(SUCCESS_REGISTERING_FINGERPRINT_ACCESS);
    return true;
}

/**
 * @brief Delete a fingerprint access control
 *
 * This function attempts to delete a fingerprint model from the fingerprint sensor and
 * also removes the associated fingerprint data from the SD card.
 *
 * @param keyAccessId The Key Access ID that was associated with the fingerprint ID from the server to be deleted at SD Card
 * @return true if the fingerprint was successfully deleted from both the sensor and the SD card;
 *         false otherwise.
 */
bool FingerprintService::deleteFingerprint(const char *keyAccessId) {
    ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Deleting Fingerprint User! Key Access ID = %s", keyAccessId);

    // Get the fingerprintId from the SD Card
    int fingerprintId = _sdCardModule->getFingerprintIdByKeyAccessId(keyAccessId);
    if (fingerprintId <= 0) {
        return handleDeleteError(FAILED_TO_RETRIEVE_KEYACCESSID_FROM_SDCARD, "Failed to retrieve user data for Key Access ID!");
    }

    // If success there is key access ID associated with fingerprint, then delete them from the sensor and from the SD Card
    ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Deleting Fingerprint ID %d from sensor! ", fingerprintId);
    bool deleteFingerprintResultSensor = _fingerprintSensor->deleteFingerprintModel(fingerprintId);

    if (deleteFingerprintResultSensor) {
        ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Fingerprint model deleted successfully from sensor for FingerprintID: %d", fingerprintId);
        bool deleteFingerprintSDCard = _sdCardModule->deleteFingerprintFromSDCard(keyAccessId);

        if (!deleteFingerprintSDCard) {
            return handleDeleteError(FAILED_DELETE_FINGERPRINT_ACCESS_FROM_SD_CARD, "Failed to delete Fingerprint from SD card!");
        }

        // Prepare the data payload
        ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Fingerprint deleted from SD card successfully for FingerprintID: %d", fingerprintId);
        sendbleNotification(SUCCESS_DELETING_FINGERPRINT_ACCESS);
        return true;

    } else {
        return handleDeleteError(FAILED_TO_DELETE_FINGERPRINT_MODEL, "Failed to delete fingerprint model from sensor for FingerprintID: %d");
    }
}

/**
 * @brief Delete a fingerprint access control under a user
 *
 * This function to delete the all the fingerprint key access of a user. Will delete both fingerprint model
 * on the sensor and on the SD Card of the user fingerprint
 *
 * @param visitorID The Visitor ID of the user
 * @return true if the fingerprint was successfully deleted from both the sensor and the SD card;
 *         false otherwise.
 */
bool FingerprintService::deleteFingerprintsUser(const char *visitorId) {
    ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Deleting Fingerprint User! Visitor ID = %s", visitorId);

    // Get all the fingerprintId under a user from the SD Card
    std::vector<int> userFingerprintIds = _sdCardModule->getFingerprintIdsByVisitorId(visitorId);
    if (!userFingerprintIds.empty()) {
        // Now delete the user data in the SD Card
        // Don't want to delete the model first if the SD Card is failed
        if (_sdCardModule->deleteFingerprintsUserFromSDCard(visitorId)) {
            ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Successfully deleted user data for Visitor ID = %s from SD Card", visitorId);

            // Will not care whatever the outcome, true or false will move on to delete the user
            ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Found %d fingerprints for Visitor ID = %s. Deleting fingerprint models.", userFingerprintIds.size(), visitorId);
            for (int id : userFingerprintIds) {
                if (_fingerprintSensor->deleteFingerprintModel(id)) {
                    ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Fingerprint ID %d deleted successfully", id);
                } else {
                    ESP_LOGW(FINGERPRINT_SERVICE_LOG_TAG, "Failed to delete Fingerprint ID %d", id);
                }
            }
            sendbleNotification(SUCCESS_DELETING_FINGERPRINTS_USER);
            return true;
        } else {
            ESP_LOGE(FINGERPRINT_SERVICE_LOG_TAG, "Failed to delete user data for Visitor ID = %s from SD Card", visitorId);
            sendbleNotification(FAILED_TO_DELETE_FINGERPRINTS_USER);
            return false;
        }
    } else {
        ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "No fingerprints found for Visitor ID = %s. Perhaps already deleted. But will still return false", visitorId);
        sendbleNotification(NO_FINGERPRINTS_FOUND_UNDER_USER);
        return false;
    }
}

/**
 * @brief Delete all the fingerprint access control on the sensor
 *
 * This function to delete the all the fingerprint model in the sensor  
 *
 * @return true if the all fingerprint model was successfully deleted from the sensor;
 *         false otherwise.
 */
bool FingerprintService::deleteAllFingerprintModel(){
    ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Deleting All Fingerprint Models!");

    if(_fingerprintSensor->deleteAllFingerprintModel()){
        ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Successfully deleted all the fingerprint model from the sensor");
        sendbleNotification(SUCCESS_DELETING_ALL_FINGERPRINTS_MODEL);
        return true;
    }

    ESP_LOGE(FINGERPRINT_SERVICE_LOG_TAG, "Failed to delete all the fingerprints model from the sensor");
    sendbleNotification(FAILED_DELETING_ALL_FINGERPRINTS_MODEL);
    return false;
}

/**
 * @brief Deletes the Fingerprint key access file
 *
 * Deletes the `/fingerprints.json` file from the SD card.   
 *
 * @return true if the file was deleted successfully, false otherwise.
 */
bool FingerprintService::deleteFingerprintAccessFile(){
    ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Deleting the Fingerprint .json Key Access file!");

    if(_sdCardModule->deleteAccessJsonFile(LockType::FINGERPRINT)){
        ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Successfully deleted the fingerprint key access file");
        sendbleNotification(SUCCESS_DELETING_FINGERPRINT_ACCESS_FILE);
        return true;
    }
    ESP_LOGE(FINGERPRINT_SERVICE_LOG_TAG, "Failed to delete the fingerprints key access file");
    sendbleNotification(FAILED_TO_DELETE_FINGERPRINT_ACCESS_FILE);
    return false;
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

            // Get the key access Id of that Fingerprint ID
            std::string *keyAccessId = _sdCardModule->getKeyAccessIdByFingerprintId(isRegsiteredModel);

            if(keyAccessId != nullptr){
                // Send the access history without waiting the response
                FingerprintQueueRequest msg;
                msg.state = AUTHENTICATE_FP;
                msg.fingerprintId = isRegsiteredModel;
                snprintf(msg.keyAccessId, sizeof(msg.keyAccessId), "%s", keyAccessId->c_str());

                if (xQueueSend(_fingerprintQueueRequest, &msg, portMAX_DELAY) != pdPASS) {
                    ESP_LOGE(FINGERPRINT_SERVICE_LOG_TAG, "Failed to send Fingerprint message to WiFi queue!");
                }

                return true;
            }
        }

        ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Fingerprint Model ID %d is Registered on Sensor, but not appear in stored data. Cannot open the Door Lock!", isRegsiteredModel);
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
    ESP_LOGI(FINGERPRINT_SERVICE_LOG_TAG, "Sending Fingerprint Service Action Result to BLE Notification, Status Code: %d", statusCode);
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
 * @param keyAccessId Key Access ID to be used for cleanup (can be nullptr if not applicable).
 * @param message Error message to log and send in the notification.
 * @param cleanup If true and visitorId is provided, will send a REMOVE_FP request to the server.
 * @return Always returns false
 */
bool FingerprintService::handleError(int statusCode, const char* username, const char* keyAccessId, const char* message, bool cleanup) {
    ESP_LOGE(FINGERPRINT_SERVICE_LOG_TAG, "Error Code: %d, %s", statusCode, message);
    sendbleNotification(statusCode);

    if (cleanup && keyAccessId) {
        FingerprintQueueRequest msg;
        msg.state = DELETE_FP;
        snprintf(msg.keyAccessId, sizeof(msg.keyAccessId), "%s", keyAccessId);
        snprintf(msg.username, sizeof(msg.username), "%s", username);
        snprintf(msg.vehicleInformationNumber, sizeof(msg.vehicleInformationNumber), "%s", VIN);

        if (xQueueSend(_fingerprintQueueRequest, &msg, portMAX_DELAY) != pdPASS) {
            ESP_LOGE(FINGERPRINT_SERVICE_LOG_TAG, "Failed to send cleanup request for Key Access ID: %s", keyAccessId);
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
 * @param message Error message to log and send in the notification.
 * @return Always returns false  
 */
bool FingerprintService::handleDeleteError(int statusCode, const char* message) {
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
