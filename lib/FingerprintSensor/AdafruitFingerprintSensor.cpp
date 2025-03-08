#define LOG_TAG "ADAFRUIT_FINGERPRINT"
#include "esp_log.h"
#include "AdafruitFingerprintSensor.h"

AdafruitFingerprintSensor::AdafruitFingerprintSensor()
    : _serial(UART_NR), _fingerprintSensor(&_serial) {
        setup();
    }

bool AdafruitFingerprintSensor::setup(){
    _serial.begin(BAUD_RATE_FINGERPRINT, SERIAL_8N1, RX_PIN, TX_PIN);

    uint8_t retries = 1;
    uint8_t maxRetries = 5;
    unsigned long backoffTime = 1000;
    
    while (retries <= maxRetries){
        if (_fingerprintSensor.verifyPassword()) {
            ESP_LOGI(LOG_TAG, "Fingerprint sensor has been found! Proceed");
            return true;
        } else {
            ESP_LOGI(LOG_TAG, "Fingerprint sensor has not been found! Retrying %d .....", retries);

            // Exponential backoff: double the wait time after each failure
            vTaskDelay(backoffTime);
            backoffTime *= 2;
            retries++;
        }
    }
    // If we reached here, all retries have failed
    ESP_LOGI(LOG_TAG, "Failed to initialize Fingerprint Sensor after multiple attempts! Reboot...");
    ESP.restart();
    return false;
}

bool AdafruitFingerprintSensor::addFingerprintModel(int id) {
    ESP_LOGI(LOG_TAG, "Fingerprint registration with Relevant ID %d", id);
    while (_fingerprintSensor.getImage() != FINGERPRINT_OK);
    if (_fingerprintSensor.image2Tz(1) != FINGERPRINT_OK) {
        ESP_LOGI(LOG_TAG, "Failed to convert first Fingerprint image scan!");
        return false;
    }

    ESP_LOGI(LOG_TAG, "Remove fingerprint from Sensor....");
    vTaskDelay(2000);

    while (_fingerprintSensor.getImage() != FINGERPRINT_NOFINGER);
    ESP_LOGI(LOG_TAG, "Please hold the same Fingerprint back to sensor...");
    while (_fingerprintSensor.getImage() != FINGERPRINT_OK);
    if (_fingerprintSensor.image2Tz(2) != FINGERPRINT_OK) {
      ESP_LOGI(LOG_TAG, "Failed to convert second Fingerprint image scan!");
      return false;
    }

    if (_fingerprintSensor.createModel() != FINGERPRINT_OK) {
        ESP_LOGI(LOG_TAG, "Failed to make the Fingerprint model!");
        return false;
    }

    if (_fingerprintSensor.storeModel(id) == FINGERPRINT_OK) {
        ESP_LOGI(LOG_TAG, "Fingerprint successfully captured with ID %d", id);
    }

    return true;
}

bool AdafruitFingerprintSensor::deleteFingerprintModel(int id) {
    ESP_LOGI(LOG_TAG, "Deleting Fingerprint Model with ID %d", id);
    
    if(id == 0){
        if(_fingerprintSensor.emptyDatabase() == FINGERPRINT_OK){
            ESP_LOGI(LOG_TAG, "Successfully deleted all Fingerprint Model from sensor!");
            return true;
        }else{
            ESP_LOGI(LOG_TAG, "Unsuccessfull delete all Fingerprint Model from sensor!");
            return false;
        }
    }else{
        if(_fingerprintSensor.deleteModel(id) == FINGERPRINT_OK){
            ESP_LOGI(LOG_TAG, "Fingerprint model with ID %d successfully deleted from sensor!", id);
            return true;
        }else{
            ESP_LOGI(LOG_TAG, "Failed to delete Fingerprint Model with ID %d", id);
            return false;
        }
    }
}