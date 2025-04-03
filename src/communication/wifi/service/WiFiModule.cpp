#include "WiFiModule.h"
#include <esp_log.h>
#include <nvs_flash.h>
#include "../src/config/Config.h"

#define HTTP_SERVICE_LOG_TAG "HTTP_SERVICE"
WiFiClient client; // Declare a WiFiClient object for non-secure communication
WiFiModule::WiFiModule()
{
    connectWifi();
}

bool WiFiModule::connectWifi()
{

    // Inisialisasi NVS sebelum mengaktifkan WiFi
    if (nvs_flash_init() != ESP_OK)
    {
        Serial.println("⚠️ NVS gagal diinisialisasi! Menghapus dan mencoba lagi...");
        nvs_flash_erase();
        nvs_flash_init();
    }
    ESP_LOGI(HTTP_SERVICE_LOG_TAG, "Connecting to WiFi...");

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int retry_count = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        retry_count++;
        ESP_LOGI(HTTP_SERVICE_LOG_TAG, "Attempting to connect... (%d) : %s %s", retry_count, WIFI_SSID, WIFI_PASSWORD);

        if (retry_count > 20)
        {
            ESP_LOGE(HTTP_SERVICE_LOG_TAG, "Failed to connect to WiFi");
            return false;
        }
    }

    ESP_LOGI(HTTP_SERVICE_LOG_TAG, "Connected to WiFi. IP Address: %s", WiFi.localIP().toString().c_str());
    return true;
}

bool WiFiModule::isConnected()
{
    return WiFi.status() == WL_CONNECTED;
}

std::string WiFiModule::sendPostRequest(const std::string &url, const std::string &payload)
{
    if (!isConnected())
    {
        return "WiFi not connected!";
    }

    HTTPClient http;
    http.begin(client, url.c_str());
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(payload.c_str());

    std::string response;
    if (httpResponseCode > 0)
    {
        response = http.getString().c_str();
        ESP_LOGI(HTTP_SERVICE_LOG_TAG, "HTTP Response: %d, %s", httpResponseCode, response.c_str());
    }
    else
    {
        ESP_LOGE(HTTP_SERVICE_LOG_TAG, "Error on sending POST request: %s", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
    return response;
}

std::string WiFiModule::sendGetRequest(const std::string &url)
{
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
        ESP_LOGI(HTTP_SERVICE_LOG_TAG, "HTTP Response: %d, %s", httpResponseCode, response.c_str());
    }
    else
    {
        ESP_LOGE(HTTP_SERVICE_LOG_TAG, "Error on sending GET request: %s", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
    return response;
}

std::string WiFiModule::sendDeleteRequest(const std::string &url)
{
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
        ESP_LOGI(HTTP_SERVICE_LOG_TAG, "HTTP Response: %d, %s", httpResponseCode, response.c_str());
    }
    else
    {
        ESP_LOGE(HTTP_SERVICE_LOG_TAG, "Error on sending DELETE request: %s", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
    return response;
}
