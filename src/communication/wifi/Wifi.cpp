#include "Wifi.h"
#include <esp_log.h>
#include <HTTPClient.h>
#include <WiFiClient.h> // Declare a WiFiClient object for non-secure communication

WiFiClient client;
HTTPClient http;
#define WIFI_LOG_TAG "WIFI"

Wifi::Wifi(const char* ssid, QueueHandle_t statusQueue)
    : _apName(ssid), _wifiStatusQueue(statusQueue) {}

/// @brief Initialize the WiFi connection where using WiFi Manager to manage the lifectcle of the WiFi
void Wifi::init() {
    ESP_LOGI(WIFI_LOG_TAG, "Initializing WiFi Manager Server and Service...");
    WiFi.mode(WIFI_STA);
    _wifiManager.autoConnect(_apName);

    ESP_LOGI(WIFI_LOG_TAG, "ESP has been successfully connected to Wifi/Internet");
    WiFi.begin(); // Save the credential so automatically will connect to last credential that has been connected
}

/**
 * @brief Getting the WiFi connection status of the device to the internet
 * 
 * @return `true` if the device is connected to the Internet, `false` otherwise.
 */ 
bool Wifi::isConnected() {
    ESP_LOGI(WIFI_LOG_TAG, "Checking WiFi connection status: %s",
        WiFi.status() == WL_CONNECTED ? "CONNECTED" : "NOT CONNECTED");
    return WiFi.status() == WL_CONNECTED;
}

/**
 * @brief Attempts to reconnect to WiFi if not already connected.
 *
 * Logs a warning if WiFi is not connected, then triggers auto-connect.
 * Sends the new connection status to the status queue.
 *
 * @return true if reconnection was attempted, false if already connected.
 */
bool Wifi::reconnect() {
    if (!isConnected()){
        ESP_LOGW(WIFI_LOG_TAG, "WiFi not connected. Attempting to reconnect...");
        _wifiManager.autoConnect(_apName);
        bool connected = isConnected();
        updateStatus(connected);
        ESP_LOGI(WIFI_LOG_TAG, "Reconnection result: %s", connected ? "SUCCESS" : "FAILED");
        return true;
    }
    ESP_LOGI(WIFI_LOG_TAG, "Already connected. No reconnection needed.");
    return false;
}

/**
 * @brief Sends the WiFi connection status to the FreeRTOS queue.
 *
 * @param status Current connection status (true if connected).
 */
void Wifi::updateStatus(bool status){
    if (_wifiStatusQueue != NULL) {
        xQueueSendToBack(_wifiStatusQueue, &status, portMAX_DELAY);
        ESP_LOGD(WIFI_LOG_TAG, "Updated WiFi status queue with value: %s", status ? "true" : "false");
    } else {
        ESP_LOGW(WIFI_LOG_TAG, "WiFi status queue is NULL. Cannot update status.");
    }
}

/**
 * @brief Gets the SSID of the connected WiFi network.
 *
 * @return const char* to the current SSID.
 */
const char* Wifi::getSSID(){
    ESP_LOGI(WIFI_LOG_TAG, "Retrieving SSID: %s", WiFi.SSID().c_str());
    return WiFi.SSID().c_str();
}

/**
 * @brief Gets the local IP address of the device.
 *
 * @return const char* representing the IP address.
 */
const char* Wifi::getIpAddress() {
    static char ipStr[16];
    snprintf(ipStr, sizeof(ipStr), "%u.%u.%u.%u",
             WiFi.localIP()[0], WiFi.localIP()[1], 
             WiFi.localIP()[2], WiFi.localIP()[3]);
    ESP_LOGI(WIFI_LOG_TAG, "Device IP address: %s", ipStr);
    return ipStr;
}

/**
 * @brief Gets the current WiFi connection state.
 *
 * @return wl_status_t representing the WiFi connection state.
 */
wl_status_t Wifi::get_state(){
    wl_status_t state = WiFi.status();
    ESP_LOGD(WIFI_LOG_TAG, "Current WiFi state: %d", static_cast<int>(state));
    return state;
}

/**
 * @brief Sending post request to API
 *
 * @param url The pointer location of the url string of the api
 * @param payload The pointer location of the payload string in form of json application 
 * @return response of the api
 */
std::string Wifi::sendPostRequest(const std::string &url, const std::string &payload){
    ESP_LOGI(WIFI_LOG_TAG, "Sending POST request to URL: %s", url.c_str());
    ESP_LOGD(WIFI_LOG_TAG, "Payload: %s", payload.c_str());

    http.begin(client, url.c_str());
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(payload.c_str());

    std::string response;
    response = http.getString().c_str();

    if (httpResponseCode > 0) ESP_LOGI(WIFI_LOG_TAG, "HTTP Response: %d, %s", httpResponseCode, response.c_str());
    else ESP_LOGE(WIFI_LOG_TAG, "Error on sending POST request: %s", http.errorToString(httpResponseCode).c_str());

    http.end();
    return response;
}

/**
 * @brief Sending get request to API
 *
 * @param url The pointer location of the url string of the api
 * @param payload The pointer location of the payload string in form of json application 
 * @return response of the api
 */
std::string Wifi::sendGetRequest(const std::string &url){
    if (!isConnected()) {
        return "WiFi not connected!";
    }

    http.begin(client, url.c_str());

    int httpResponseCode = http.GET();

    std::string response;
    if (httpResponseCode > 0) {
        response = http.getString().c_str();
        ESP_LOGI(WIFI_LOG_TAG, "HTTP Response: %d, %s", httpResponseCode, response.c_str());
    }
    else {
        ESP_LOGE(WIFI_LOG_TAG, "Error on sending GET request: %s", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
    return response;
}

/**
 * @brief Sending delete request to API
 *
 * @param url The pointer location of the url string of the api
 * @param payload The pointer location of the payload string in form of json application 
 * @return response of the api
 */
std::string Wifi::sendDeleteRequest(const std::string &url){
    if (!isConnected()) {
        return "WiFi not connected!";
    }
    http.begin(client, url.c_str());
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.sendRequest("DELETE");

    std::string response;
    if (httpResponseCode > 0) {
        response = http.getString().c_str();
        ESP_LOGI(WIFI_LOG_TAG, "HTTP Response: %d, %s", httpResponseCode, response.c_str());
    }
    else {
        ESP_LOGE(WIFI_LOG_TAG, "Error on sending DELETE request: %s", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
    return response;
}