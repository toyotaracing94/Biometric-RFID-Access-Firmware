#define WIFI_SERVICE_LOG_TAG "WIFI_SERVICE"
#include "WifiService.h"
#include <esp_log.h>

WifiService::WifiService(BLEModule* bleModule, OTA *otaModule, SDCardModule *sdCardModule)
    :_bleModule(bleModule), _otaModule(otaModule), _sdCardModule(sdCardModule) {
    // Create new object of Wifi for the Wifi Tasks
    _wifi = new Wifi();
}

bool WifiService::setup() {
    ESP_LOGI(WIFI_SERVICE_LOG_TAG, "Start initializing the WiFi!");
    // Start the Wifi manager from the Wifi Service instead
    _wifi->init();
    return true;
}

/**
 * @brief Checking the WiFi Connection status
 *
 * 
 * @return `true` if device/esp32 is connected to WiFi/Internet, `false` otherwise
 */
bool WifiService::isConnected() {
    ESP_LOGI(WIFI_SERVICE_LOG_TAG, "Checking WiFi connection status.");
    return _wifi->isConnected();
}

bool WifiService::reconnect(){
    ESP_LOGI(WIFI_SERVICE_LOG_TAG, "Attempting to reconnect to WiFi...");
    return _wifi->reconnect();
}

/**
 * @brief Sends request for adding new NFC access data to the backend server via HTTP POST.
 *   
 * @param nfcrequest The NFC request data, including VIN and username.
 * @return NFCQueueResponse The NFC response
 */
NFCQueueResponse WifiService::addNFCToServer(NFCQueueRequest nfcrequest){
    ESP_LOGI(WIFI_SERVICE_LOG_TAG, 
        "Sending request for Adding new NFC data to the server. Username : %s VIN %s", nfcrequest.username, nfcrequest.vehicleInformationNumber);
    
    // For now url will be stored here first
    std::string url = "http://203.100.57.59:3000/api/v1/user-vehicle/visitor";

    // Prepare the document payload
    JsonDocument document;
    document["visitor_name"] = nfcrequest.username;
    document["vin"] = nfcrequest.vehicleInformationNumber;
    document["type"] = "RFID";

    // Serialize the json into c-string
    std::string payload;
    serializeJson(document, payload);
    
    // Send the request to the server
    std::string response = _wifi->sendPostRequest(url, payload);

    // Prepare the return statement
    NFCQueueResponse nfcResponse;
    nfcResponse.request_id = nfcrequest.request_id;
    snprintf(nfcResponse.response, sizeof(nfcResponse.response), "%s", response.c_str());

    return nfcResponse;
}

/**
 * @brief Sends request for delete  NFC access data to the backend server via HTTP DELETE.
 *  
 * @param nfcrequest The NFC request data, including VIN and username.
 * @return NFCQueueResponse The NFC response
 */
NFCQueueResponse WifiService::deleteNFCFromServer(NFCQueueRequest nfcrequest){
    ESP_LOGI(WIFI_SERVICE_LOG_TAG, "Sending request for deleting NFC data to the server. Visitor ID : %s", nfcrequest.visitorId);
    
    // For now url will be stored here first
    std::string url = "http://203.100.57.59:3000/api/v1/user-vehicle/visitor/" + std::string(nfcrequest.visitorId);
    
    // Send the request to the server
    std::string response = _wifi->sendDeleteRequest(url);
    
    // Prepare the return statement
    NFCQueueResponse nfcResponse;
    nfcResponse.request_id = nfcrequest.request_id;
    snprintf(nfcResponse.response, sizeof(nfcResponse.response), "%s", response.c_str());
    
    return nfcResponse;
}

/**
 * @brief Sends request for updating/adding new NFC access history the backend server via HTTP POST.
 *  
 * @param nfcrequest The NFC request data, including VIN and username.
 * @return NFCQueueResponse The NFC response
 */
NFCQueueResponse WifiService::authenticateAccessNFCToServer(NFCQueueRequest nfcrequest){
    ESP_LOGI(WIFI_SERVICE_LOG_TAG, "Sending request for addding history NFC data access to the server. Visitor ID %s", nfcrequest.visitorId);

    // For now url will be stored here first
    std::string url = "http://203.100.57.59:3000/api/v1/user-vehicle/visitor/activity";

    // Prepare the document payload
    JsonDocument document;
    document["visitor_id"] = nfcrequest.visitorId;

    // Serialize the json into c-string
    std::string payload;
    serializeJson(document, payload);

    // Send the requesrt to the server
    std::string response = _wifi->sendPostRequest(url, payload);

    // Prepare the return statement
    NFCQueueResponse nfcResponse;
    nfcResponse.request_id = nfcrequest.request_id;
    snprintf(nfcResponse.response, sizeof(nfcResponse.response), "%s", response.c_str());

    return nfcResponse;
}

/**
 * @brief Sends request for adding new Fingerprint access data to the backend server via HTTP POST.
 *  
 * @param nfcrequest The Fingerprint request data, including VIN and username.
 * @return FingerprintQueueResponse The Fingerprint response
 */
FingerprintQueueResponse WifiService::addFingerprintToServer(FingerprintQueueRequest fingerprintRequest){
    ESP_LOGI(WIFI_SERVICE_LOG_TAG, "Sending request for adding new Fingerprint data to the server");
    
    // For now url will be stored here first
    std::string url = "http://203.100.57.59:3000/api/v1/user-vehicle/visitor";

    // Prepare the document payload
    JsonDocument document;
    document["vin"] = fingerprintRequest.vehicleInformationNumber;
    document["visitor_name"] = fingerprintRequest.username;
    document["type"] = "Fingerprint";

    // Serialize the json into c-string
    std::string payload;
    serializeJson(document, payload);
    
    // Send the request to the server
    std::string response = _wifi->sendPostRequest(url, payload);

    // Prepare the return statement
    FingerprintQueueResponse fingerprintResponse;
    fingerprintResponse.request_id = fingerprintRequest.request_id;
    snprintf(fingerprintResponse.response, sizeof(fingerprintResponse.response), "%s", response.c_str());

    return fingerprintResponse;
}

/**
 * @brief Sends request for delete Fingerprint access data to the backend server via HTTP DELETE.
 *  
 * @param fingerprintRequest The Fingerprint request data, including VIN and username.
 * @return FingerprintQueueRequest The Fingerprint response
 */
FingerprintQueueResponse WifiService::deleteFingerprintFromServer(FingerprintQueueRequest fingerprintRequest){
    ESP_LOGI(WIFI_SERVICE_LOG_TAG, "Sending request for deleting Fingerprint data to the server");

    // For now url will be stored here first
    std::string url = "http://203.100.57.59:3000/api/v1/user-vehicle/visitor/" + std::string(fingerprintRequest.visitorId);

    // Send the request to the server
    std::string response = _wifi->sendDeleteRequest(url);

    // Prepare the return statement
    FingerprintQueueResponse fingerprintResponse;
    fingerprintResponse.request_id = fingerprintRequest.request_id;
    snprintf(fingerprintResponse.response, sizeof(fingerprintResponse.response), "%s", response.c_str());

    return fingerprintResponse;
}

/**
 * @brief Sends request for updating/adding new Fingerprint access history the backend server via HTTP POST.  
 *
 * @param fingerprintRequest The Fingerprint request data, including VIN and username.
 * @return FingerprintQueueRequest The Fingerprint response
 */
FingerprintQueueResponse WifiService::authenticateAccessFingerprintToServer(FingerprintQueueRequest fingerprintRequest){
    ESP_LOGI(WIFI_SERVICE_LOG_TAG, "Sending request for addding history Fingerprint data access to the server");

    // For now url will be stored here first
    std::string url = "http://203.100.57.59:3000/api/v1/user-vehicle/visitor/activity";

    // Prepare the document payload
    JsonDocument document;
    document["visitor_id"] = fingerprintRequest.visitorId;

    // Serialize the json into c-string
    std::string payload;
    serializeJson(document, payload);
    
    // Send the requesrt to the server
    std::string response = _wifi->sendPostRequest(url, payload);

    // Prepare the return statement
    FingerprintQueueResponse fingerprintResponse;
    fingerprintResponse.request_id = fingerprintRequest.request_id;
    snprintf(fingerprintResponse.response, sizeof(fingerprintResponse.response), "%s", response.c_str());

    return fingerprintResponse;
}

/**
 * @brief Start the configuration of the OTA Service such as the callback for each OTA progress
 * and starts the ArduinoOTA service
 *   
 */
void WifiService::beginOTA(){
    ESP_LOGI(WIFI_SERVICE_LOG_TAG, "Start initializing the OTA Service!");

    // Ensure first that devices is already connected to some WiFi
    // that came from the WiFi Manager settings
    while (WiFi.status() != WL_CONNECTED){
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    _otaModule->init();
}

/**
 * @brief Start the configuration of the OTA Service such as the callback for each OTA progress
 * and starts the ArduinoOTA service
 *   
 */
void WifiService::handleOTA(){
    ESP_LOGD(WIFI_SERVICE_LOG_TAG, "Running the OTA Service!");

    // Run this on the loop
    _otaModule->handleOTA();
}


