#define LOG_TAG "ADAFRUIT_FINGERPRINT"
#include <esp_log.h>
#include "AdafruitFingerprintSensor.h"

AdafruitFingerprintSensor::AdafruitFingerprintSensor()
    : _serial(UART_NR), _fingerprintSensor(&_serial) {
        setup();
    }

bool AdafruitFingerprintSensor::setup(){
    ESP_LOGI(LOG_TAG, "Start Fingerprint Sensor Setup!");
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

int AdafruitFingerprintSensor::getFingerprintIdModel(){
    if (_fingerprintSensor.getImage() == FINGERPRINT_OK){
        ESP_LOGI(LOG_TAG, "Fingerprint image captured successfully.");
        
        ESP_LOGD(LOG_TAG, "Prepare the next stage for convert the image to feature model");
        if(_fingerprintSensor.image2Tz() == FINGERPRINT_OK && _fingerprintSensor.fingerSearch() == FINGERPRINT_OK){
            uint8_t fingerprintId = _fingerprintSensor.fingerID;
            ESP_LOGI(LOG_TAG, "Fingerprint matched! Detected ID: %d", fingerprintId);
            return (int)fingerprintId;
        }else{
            ESP_LOGW(LOG_TAG, "Fingerprint match failed or image conversion error.");
            return FAILED_IMAGE_CONVERSION_ERROR;
        }
    }
    return FAILED_TO_GET_FINGERPRINT_MODEL_ID;
}

bool AdafruitFingerprintSensor::addFingerprintModel(int id) {
    ESP_LOGI(LOG_TAG, "Fingerprint registration with Fingerprint ID %d", id);
    while (_fingerprintSensor.getImage() != FINGERPRINT_OK);
    if (_fingerprintSensor.image2Tz(1) != FINGERPRINT_OK) {
        ESP_LOGI(LOG_TAG, "Failed to convert first Fingerprint image scan!");
        return false;
    }

    ESP_LOGI(LOG_TAG, "Remove fingerprint from Sensor....");
    vTaskDelay(2000 / portTICK_PERIOD_MS);

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

void AdafruitFingerprintSensor::toggleSuccessFingerprintLED(){
    ESP_LOGD(LOG_TAG, "Toggling success fingerprint LED!");
    _fingerprintSensor.LEDcontrol(FINGERPRINT_LED_BREATHING, 128, FINGERPRINT_LED_BLUE, 1);
}

void AdafruitFingerprintSensor::toggleFailedFingerprintLED(){
    ESP_LOGD(LOG_TAG, "Toggling failed fingerprint LED!");
    _fingerprintSensor.LEDcontrol(FINGERPRINT_LED_FLASHING, 5, FINGERPRINT_LED_BLUE, 5);
}