#define NFC_SENSOR_LOG_TAG "ADAFRUIT_PN532_RFID"
#include <esp_log.h>
#include "AdafruitNFCSensor.h"

AdafruitNFCSensor::AdafruitNFCSensor()
    : _pn532Sensor(SDA_PIN, SCL_PIN){
        setup();
    }


/**
 * @brief  Adafruit NFC Sensor setup
 *
 * Initializes the Adafruit NFC sensor and establishes communication with the sensor.
 *
 * @return
 *     - true  : The NFC sensor has been successfully initialized and is ready to use.
 *     - false : The NFC sensor could not be initialized after the maximum retry attempts.
 */
bool AdafruitNFCSensor::setup(){
    ESP_LOGI(NFC_SENSOR_LOG_TAG, "Start NFC Sensor Setup!");

    // Re-initialize I2C and reset from any previous I2C connections
    Wire.end();
    vTaskDelay(500 / portTICK_PERIOD_MS);
    Wire.begin(SDA_PIN, SCL_PIN);

    // Add delay to wait sensor to be ready and timing issues for the watchdog
    vTaskDelay(200 / portTICK_PERIOD_MS);
    _pn532Sensor.begin();

    uint8_t retries = 1;
    uint8_t maxRetries = 5;
    unsigned long backoffTime = 1000;
    uint32_t firmwareVersion;

    while (retries <= maxRetries){
        firmwareVersion = _pn532Sensor.getFirmwareVersion();
        if (firmwareVersion) {
            ESP_LOGI(NFC_SENSOR_LOG_TAG, "Adafruit PN532 Firmware version %d", firmwareVersion);
            ESP_LOGI(NFC_SENSOR_LOG_TAG, "NFC Sensor has been found! Proceed...");
            return true;
        } else {
            ESP_LOGI(NFC_SENSOR_LOG_TAG,"Cannot find the NFC Sensor PN532! Retrying %d...", retries);
      
            // Exponential backoff: Double the backoff time after each failure
            vTaskDelay(backoffTime / portTICK_PERIOD_MS);
            backoffTime *= 2;
            retries++;
        }
    }

    // If we reached here, all retries have failed
    ESP_LOGI(NFC_SENSOR_LOG_TAG, "Failed to initialize NFC Sensor after multiple attempts! Reboot...");
    ESP.restart();
    return false;
}
 
/**
 * @brief  Reads the UID of an NFC card.
 *
 * This function reads the UID of an NFC card, formats it as a hexadecimal string, 
 * and returns the formatted string representing the card's UID.
 * 
 * @param timeout  The timeout period for reading the NFC card (in milliseconds).
 * 
 * @return A pointer to a static char object containing the formatted NFC card UID. 
 *         Returns an empty string if no card is detected or if the read fails.
 */
char* AdafruitNFCSensor::readNFCCard(uint16_t timeout) {
    uint8_t uid[7];
    uint8_t uidLength;
    static char uid_card[255];

    // Clear the buffer before reading the new card
    // TODO : Find a way rather resort than this where it use static just for this 
    memset(uid_card, 0, sizeof(uid_card));

    bool readResult = _pn532Sensor.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, timeout);
    if (readResult) {
        uint8_t index = 0;
        for (uint8_t i = 0; i < uidLength; i++) {
            index += snprintf(uid_card + index, sizeof(uid_card) - index, "%02X", uid[i]);
            if (i < uidLength - 1) {
                index += snprintf(uid_card + index, sizeof(uid_card) - index, ":");
            }
        }
        ESP_LOGI(NFC_SENSOR_LOG_TAG, "Found NFC tag with UID: %s", uid_card);
    } else{
        ESP_LOGD(NFC_SENSOR_LOG_TAG, "No NFC card detected. Perhaps timeout possibly");
    }
    return uid_card;
}