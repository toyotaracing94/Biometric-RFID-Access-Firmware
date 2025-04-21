#include "ota.h"
#include <esp_log.h>

#define OTA_LOG_TAG "OTA"

OTA::OTA(){}

/**
 * @brief Initialize the OTA configurations
 * 
 */
void OTA::init(){
    ArduinoOTA.onStart([]() {
        const char* type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        ESP_LOGI(OTA_LOG_TAG, "Start updating %s", type);
    });

    ArduinoOTA.onEnd([]() {
        ESP_LOGI(OTA_LOG_TAG, "OTA update finished.");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        int percent = (progress * 100) / total;
        ESP_LOGI(OTA_LOG_TAG, "OTA Progress: %d%%", percent);
    });

    ArduinoOTA.onError([](ota_error_t error) {
        const char* errorMsg = "Unknown error";
        switch (error) {
            case OTA_AUTH_ERROR:    errorMsg = "Auth Failed"; break;
            case OTA_BEGIN_ERROR:   errorMsg = "Begin Failed"; break;
            case OTA_CONNECT_ERROR: errorMsg = "Connect Failed"; break;
            case OTA_RECEIVE_ERROR: errorMsg = "Receive Failed"; break;
            case OTA_END_ERROR:     errorMsg = "End Failed"; break;
            default:                break;
        }
        ESP_LOGE(OTA_LOG_TAG, "OTA Error [%d]: %s", error, errorMsg);
    });

    
    ArduinoOTA.begin();
    ESP_LOGI(OTA_LOG_TAG, "OTA service started. Ready to receive updates.");
}

/**
 * @brief Wrapper for run the OTA Service in the loop
 *   
 */
void OTA::handleOTA(){
    // Start the service of ArduinoOTA to listen for any request of OTA updates
    ArduinoOTA.handle();
}