#include "Wifi.h"
#include <esp_log.h>
#include <HTTPClient.h>
#include <WiFiClient.h> // Declare a WiFiClient object for non-secure communication

WiFiClient client;
#define WIFI_LOG_TAG "WIFI"


Wifi::Wifi(const char* ssid, QueueHandle_t statusQueue)
    : _apName(ssid), _wifiStatusQueue(statusQueue) {}

void Wifi::init() {
    WiFi.mode(WIFI_STA);
    _wifiManager.autoConnect(_apName, "password");

    ESP_LOGI(WIFI_LOG_TAG, "Connected to Wifi");
    WiFi.begin();
}

bool Wifi::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

bool Wifi::reconnect() {
    if (! isConnected()){
        ESP_LOGW(WIFI_LOG_TAG, "WiFi not connected. Reconnecting");
        _wifiManager.autoConnect(_apName, "password");
        updateStatus(isConnected());
        return true;
    }
    return false;
}

void Wifi::updateStatus(bool status){
    if (_wifiStatusQueue != NULL) {
        xQueueSendToBack(_wifiStatusQueue, &status, portMAX_DELAY);
    }
}

const char* Wifi::getSSID(){
    return WiFi.SSID().c_str();
}

const char* Wifi::getIpAddress() {
    static char ipStr[16];
    snprintf(ipStr, sizeof(ipStr), "%u.%u.%u.%u", 
             WiFi.localIP()[0], WiFi.localIP()[1], 
             WiFi.localIP()[2], WiFi.localIP()[3]);
    return ipStr;
}

wl_status_t Wifi::get_state(){
    return WiFi.status();
}

std::string Wifi::sendPostRequest(const std::string &url, const std::string &payload){
    HTTPClient http;
    http.begin(client, url.c_str());
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(payload.c_str());

    std::string response;
    if (httpResponseCode > 0)
    {
        response = http.getString().c_str();
        ESP_LOGI(WIFI_LOG_TAG, "HTTP Response: %d, %s", httpResponseCode, response.c_str());
    }
    else
    {
        ESP_LOGE(WIFI_LOG_TAG, "Error on sending POST request: %s", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
    return response;
}

std::string Wifi::sendGetRequest(const std::string &url){
    if (!isConnected())
    {
        return "WiFi not connected!";
    }

    HTTPClient http;
    http.begin(client, url.c_str());

    int httpResponseCode = http.GET();

    std::string response;
    if (httpResponseCode > 0)
    {
        response = http.getString().c_str();
        ESP_LOGI(WIFI_LOG_TAG, "HTTP Response: %d, %s", httpResponseCode, response.c_str());
    }
    else
    {
        ESP_LOGE(WIFI_LOG_TAG, "Error on sending GET request: %s", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
    return response;
}

std::string Wifi::sendDeleteRequest(const std::string &url){
    if (!isConnected())
    {
        return "WiFi not connected!";
    }

    HTTPClient http;
    http.begin(client, url.c_str());
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.sendRequest("DELETE");

    std::string response;
    if (httpResponseCode > 0)
    {
        response = http.getString().c_str();
        ESP_LOGI(WIFI_LOG_TAG, "HTTP Response: %d, %s", httpResponseCode, response.c_str());
    }
    else
    {
        ESP_LOGE(WIFI_LOG_TAG, "Error on sending DELETE request: %s", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
    return response;
}