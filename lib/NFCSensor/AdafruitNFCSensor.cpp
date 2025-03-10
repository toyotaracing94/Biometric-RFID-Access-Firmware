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
 
char* AdafruitNFCSensor::readNFCCard(uint16_t timeout) {
    uint8_t uid[7];
    uint8_t uidLength;
    static char uid_card[255];

    bool readResult = _pn532Sensor.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, timeout);
    if (readResult) {
        uint8_t index = 0;
        for (uint8_t i = 0; i < uidLength; i++) {
            index += snprintf(uid_card + index, sizeof(uid_card) - index, "%02X", uid[i]);
            if (i < uidLength - 1) {
                index += snprintf(uid_card + index, sizeof(uid_card) - index, ":");
            }
        }
        ESP_LOGI(LOG_TAG, "Found NFC tag with UID: %s", uid_card);
    }
    return uid_card;
}