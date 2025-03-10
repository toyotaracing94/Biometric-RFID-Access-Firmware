#define LOG_TAG "ADAFRUIT_PN532_RFID"
#include <esp_log.h>
#include "AdafruitNFCSensor.h"

AdafruitNFCSensor::AdafruitNFCSensor()
    : _pn532Sensor(SDA_PIN, SCL_PIN){
        setup();
    }

bool AdafruitNFCSensor::setup(){
    ESP_LOGI(LOG_TAG, "Start NFC Sensor Setup!");
    Wire.begin(SDA_PIN, SCL_PIN);
    _pn532Sensor.begin();

    uint8_t retries = 1;
    uint8_t maxRetries = 5;
    unsigned long backoffTime = 1000;
    uint32_t firmwareVersion;

    while (retries <= maxRetries){
        firmwareVersion = _pn532Sensor.getFirmwareVersion();
        if (firmwareVersion) {
            ESP_LOGI(LOG_TAG, "Adafruit PN532 Firmware version %d", firmwareVersion);
            ESP_LOGI(LOG_TAG, "NFC Sensor has been found! Proceed...");
            return true;
        } else {
            ESP_LOGI(LOG_TAG,"Cannot find the NFC Sensor PN532! Retrying %d...", retries);
      
            // Exponential backoff: Double the backoff time after each failure
            vTaskDelay(backoffTime / portTICK_PERIOD_MS);
            backoffTime *= 2;
            retries++;
        }
    }

    // If we reached here, all retries have failed
    ESP_LOGI(LOG_TAG, "Failed to initialize NFC Sensor after multiple attempts! Reboot...");
    ESP.restart();
    return false;
}
    