#define ADAFRUIT_SENSOR_LOG_TAG "ADAFRUIT_FINGERPRINT"
#include <esp_log.h>
#include "AdafruitFingerprintSensor.h"

AdafruitFingerprintSensor::AdafruitFingerprintSensor()
    : _serial(UART_NR), _fingerprintSensor(&_serial){
    setup();
}

/**
 * @brief  Adafruit Fingerprint setup
 *
 * Initializes the Adafruit fingerprint sensor and establishes communication with the sensor.
 *
 * @return
 *     - true  : The fingerprint sensor has been successfully initialized and is ready to use.
 *     - false : The fingerprint sensor could not be initialized after the maximum retry attempts.
 */
bool AdafruitFingerprintSensor::setup(){
    ESP_LOGI(ADAFRUIT_SENSOR_LOG_TAG, "Start Fingerprint Sensor Setup!");
    _serial.begin(BAUD_RATE_FINGERPRINT, SERIAL_8N1, RX_PIN, TX_PIN);

    uint8_t retries = 1;
    uint8_t maxRetries = 5;
    unsigned long backoffTime = 1000;

    while (retries <= maxRetries){
        if (_fingerprintSensor.verifyPassword()){
            ESP_LOGI(ADAFRUIT_SENSOR_LOG_TAG, "Fingerprint sensor has been found! Proceed");
            activateCustomPresetLED(FINGERPRINT_LED_FLASHING, 25, 5);
            return true;
        }
        else{
            ESP_LOGI(ADAFRUIT_SENSOR_LOG_TAG, "Fingerprint sensor has not been found! Retrying %d .....", retries);

            // Exponential backoff: double the wait time after each failure
            vTaskDelay(backoffTime);
            backoffTime *= 2;
            retries++;
        }
    }
    // If we reached here, all retries have failed
    ESP_LOGI(ADAFRUIT_SENSOR_LOG_TAG, "Failed to initialize Fingerprint Sensor after multiple attempts! Reboot...");
    ESP.restart();
    return false;
}

/**
 * @brief  Captures and matches fingerprint image
 *
 * This function captures a fingerprint image, converts it to a feature model,
 * and performs a fingerprint match to return the matching ID.
 * If an error occurs during any stage, it returns an error code.
 *
 * @return
 *     - On success: The fingerprint ID of the matched fingerprint.
 *     - On failure:
 *         - `FAILED_IMAGE_CONVERSION_ERROR` if the fingerprint match or image conversion failed.
 *         - `FAILED_TO_GET_FINGERPRINT_MODEL_ID` if the fingerprint image capture failed.
 */
int AdafruitFingerprintSensor::getFingerprintIdModel(){
    if (_fingerprintSensor.getImage() == FINGERPRINT_OK){
        ESP_LOGI(ADAFRUIT_SENSOR_LOG_TAG, "Fingerprint image captured successfully.");
        delay(200); // Wait before converting the image to a template
        ESP_LOGI(ADAFRUIT_SENSOR_LOG_TAG, "Prepare the next stage for convert the image to feature model");
        if (_fingerprintSensor.image2Tz() == FINGERPRINT_OK && _fingerprintSensor.fingerSearch() == FINGERPRINT_OK){
            uint8_t fingerprintId = _fingerprintSensor.fingerID;
            ESP_LOGI(ADAFRUIT_SENSOR_LOG_TAG, "Fingerprint matched! Detected ID: %d", fingerprintId);
            activateSuccessLED(FINGERPRINT_LED_BREATHING, 255, 1);
            return (int)fingerprintId;
        }
        else{
            ESP_LOGW(ADAFRUIT_SENSOR_LOG_TAG, "Fingerprint match failed or image conversion error.");
            activateFailedLED(FINGERPRINT_LED_BREATHING, 255, 1);
            return FAILED_IMAGE_CONVERSION_ERROR;
        }
    }
    return FAILED_TO_GET_FINGERPRINT_MODEL_ID;
}

/**
 * @brief  Registers a new fingerprint model to the fingerprint sensor.
 *
 * This function captures two fingerprint images, converts them into feature models,
 * creates a fingerprint model, and stores it in the sensor's memory with the given ID.
 *
 * @param id  The unique ID to associate with the new fingerprint model.
 * @param callback  Function callback to was supposed to show the progress of the fingerprint registration
 * returning the int of code status of the fingerprint state
 *
 * @return
 *      - true  If the fingerprint model was successfully created and stored.
 *      - false If an error occurred during the process.  
 */
bool AdafruitFingerprintSensor::addFingerprintModel(int id, std::function<void(int)> callback = nullptr) {
    ESP_LOGI(ADAFRUIT_SENSOR_LOG_TAG, "Fingerprint registration with Fingerprint ID %d", id);
    if (callback) callback(STATUS_FINGERPRINT_STARTED_REGISTERING);

    activateSuccessLED(FINGERPRINT_LED_BREATHING, 128, 1);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    
    ESP_LOGI(ADAFRUIT_SENSOR_LOG_TAG, "Please hold fingerprint to Sensor. First Capture of the Fingerprint Image....");
    if (callback) callback(STATUS_FINGERPRINT_PLACE_FIRST_CAPTURE);
    activateSuccessLED(FINGERPRINT_LED_FLASHING, 25, 10);
    
    if (!waitOnFingerprintForTimeout()) {
        ESP_LOGI(ADAFRUIT_SENSOR_LOG_TAG, "Timeout waiting for first fingerprint image.");
        if (callback) callback(FINGERPRINT_CAPTURE_TIMEOUT);
        activateFailedLED(FINGERPRINT_LED_BREATHING, 255, 1);
        return false;
    }

    if (callback) callback(STATUS_FINGERPRINT_PLACE_FIRST_HAS_CAPTURED);
    if (_fingerprintSensor.image2Tz(1) != FINGERPRINT_OK) {
        ESP_LOGI(ADAFRUIT_SENSOR_LOG_TAG, "Failed to convert first Fingerprint image scan! Error Code %d", FAILED_TO_CONVERT_IMAGE_TO_FEATURE);
        activateFailedLED(FINGERPRINT_LED_BREATHING, 255, 1);
        if (callback) callback(FAILED_TO_GET_FINGERPRINT_FIRST_CONVERT);
        return false;
    }

    if (callback) callback(STATUS_FINGERPRINT_FIRST_FEATURE_SUCCESS);
    vTaskDelay(500 / portTICK_PERIOD_MS);

    ESP_LOGI(ADAFRUIT_SENSOR_LOG_TAG, "Remove fingerprint from Sensor. Prepare Second Capture of the Fingerprint Image....");    
    activateCustomPresetLED(FINGERPRINT_LED_FLASHING, 25, 10);
    if (callback) callback(STATUS_FINGERPRINT_LIFT_FINGER_FROM_SENSOR);
    while (_fingerprintSensor.getImage() != FINGERPRINT_NOFINGER);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    
    ESP_LOGI(ADAFRUIT_SENSOR_LOG_TAG, "Please again hold the same Fingerprint back to sensor. Beginning to second Capture of the Fingerprint Image...");
    activateSuccessLED(FINGERPRINT_LED_FLASHING, 25, 10);
    if (!waitOnFingerprintForTimeout()) {
        ESP_LOGI(ADAFRUIT_SENSOR_LOG_TAG, "Timeout waiting for second fingerprint image.");
        if (callback) callback(FINGERPRINT_CAPTURE_TIMEOUT);
        activateFailedLED(FINGERPRINT_LED_BREATHING, 255, 1);
        return false;
    }
    if (_fingerprintSensor.image2Tz(2) != FINGERPRINT_OK) {
        ESP_LOGI(ADAFRUIT_SENSOR_LOG_TAG, "Failed to convert second Fingerprint image scan! Error Code %d", FAILED_TO_CREATE_VERIFICATION_MODEL);
        activateFailedLED(FINGERPRINT_LED_BREATHING, 128, 1);
        if (callback) callback(FAILED_TO_GET_FINGERPRINT_SECOND_CONVERT);
        return false;
    }
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    
    if (_fingerprintSensor.createModel() != FINGERPRINT_OK) {
        ESP_LOGI(ADAFRUIT_SENSOR_LOG_TAG, "Failed to make the Fingerprint model! Error Code %d", FAILED_TO_MAKE_FINGERPRINT_MODEL);
        activateFailedLED(FINGERPRINT_LED_BREATHING, 128, 1);
        if (callback) callback(FAILED_TO_MAKE_FINGERPRINT_MODEL);
        return false;
    }
    
    if (_fingerprintSensor.storeModel(id) != FINGERPRINT_OK) {
        ESP_LOGI(ADAFRUIT_SENSOR_LOG_TAG, "Failed to store the Fingerprint model! Error Code %d", FAILED_TO_STORED_FINGERPRINT_MODEL);
        activateFailedLED(FINGERPRINT_LED_BREATHING, 128, 1);
        if (callback) callback(FAILED_TO_STORED_FINGERPRINT_MODEL);
        ESP_LOGI(ADAFRUIT_SENSOR_LOG_TAG, "Fingerprint successfully captured with ID %d", id);
    }
    
    ESP_LOGI(ADAFRUIT_SENSOR_LOG_TAG, "Successfully created the Fingerprint Model and Stored them for Future Calculation Use!");
    activateSuccessLED(FINGERPRINT_LED_BREATHING, 255, 1);
    if (callback) callback(STATUS_FINGERPRINT_REGISTERING_FINGER_MODEL);
    return true;
}


/**
 * @brief  Deletes a fingerprint model from the sensor.
 *
 * This function deletes a fingerprint model from the sensor's memory.
 * If `id` is `0`, all fingerprint models are deleted.
 * Otherwise, the model with the specified `id` is deleted.
 *
 * @param id  The ID of the fingerprint model to delete, or `0` to delete all models.
 *
 * @return
 *      - true  If the fingerprint model was successfully deleted.
 *      - false If the deletion failed.
 */
bool AdafruitFingerprintSensor::deleteFingerprintModel(int id){
    ESP_LOGI(ADAFRUIT_SENSOR_LOG_TAG, "Deleting Fingerprint Model with ID %d", id);
    activateSuccessLED(FINGERPRINT_LED_BREATHING, 128, 1);

    if (id == 0){
        if (_fingerprintSensor.emptyDatabase() == FINGERPRINT_OK){
            ESP_LOGI(ADAFRUIT_SENSOR_LOG_TAG, "Successfully deleted all Fingerprint Model from sensor!");
            activateSuccessLED(FINGERPRINT_LED_BREATHING, 255, 1);
            return true;
        }
        else{
            ESP_LOGI(ADAFRUIT_SENSOR_LOG_TAG, "Unsuccessfull delete all Fingerprint Model from sensor!");
            activateFailedLED(FINGERPRINT_LED_BREATHING, 128, 1);
            return false;
        }
    }
    else{
        if (_fingerprintSensor.deleteModel(id) == FINGERPRINT_OK){
            ESP_LOGI(ADAFRUIT_SENSOR_LOG_TAG, "Fingerprint model with ID %d successfully deleted from sensor!", id);
            activateSuccessLED(FINGERPRINT_LED_BREATHING, 128, 1);
            return true;
        }
        else{
            ESP_LOGI(ADAFRUIT_SENSOR_LOG_TAG, "Failed to delete Fingerprint Model with ID %d", id);
            activateFailedLED(FINGERPRINT_LED_BREATHING, 128, 1);
            return false;
        }
    }
}

/**
 * @brief  Activates the LED color that associate to the success operation
 *
 *
 * @param control  The LED control mode (e.g., flashing or steady).
 * @param speed    The speed of the LED effect.
 * @param cycles   The number of times the LED should cycle.
 */
void AdafruitFingerprintSensor::activateSuccessLED(uint8_t control, uint8_t speed, uint8_t cycles){
    ESP_LOGI(ADAFRUIT_SENSOR_LOG_TAG, "Activating Fingerprint LED for success operation!");
    _fingerprintSensor.LEDcontrol(control, speed, FINGERPRINT_LED_BLUE, cycles);
}

/**
 * @brief  Activates the LED color that associate to the failed operation
 *
 *
 * @param control  The LED control mode (e.g., flashing or steady).
 * @param speed    The speed of the LED effect.
 * @param cycles   The number of times the LED should cycle.
 */
void AdafruitFingerprintSensor::activateFailedLED(uint8_t control, uint8_t speed, uint8_t cycles){
    ESP_LOGI(ADAFRUIT_SENSOR_LOG_TAG, "Activating Fingerprint LED for failed operation!");
    _fingerprintSensor.LEDcontrol(control, speed, FINGERPRINT_LED_RED, cycles);
}

/**
 * @brief  Activates the LED color that associate to the custom operation
 *
 *
 * @param control  The LED control mode (e.g., flashing or steady).
 * @param speed    The speed of the LED effect.
 * @param cycles   The number of times the LED should cycle.
 */
void AdafruitFingerprintSensor::activateCustomPresetLED(uint8_t control, uint8_t speed, uint8_t cycles){
    ESP_LOGI(ADAFRUIT_SENSOR_LOG_TAG, "Activating Fingerprint LED for custom preset operation!");
    _fingerprintSensor.LEDcontrol(control, speed, FINGERPRINT_LED_PURPLE, cycles);
}

/**
 * @brief  Helper function to manage the necessary time to waiting input for getting the fingerprint image input from the user  
 *
 * @param timeoutMs the timeout before operation will be canceled as user take too long. Default is 10000 ms
 */
bool AdafruitFingerprintSensor::waitOnFingerprintForTimeout(int timeoutMs = 10000){
    unsigned long start = millis();
    while (_fingerprintSensor.getImage() != FINGERPRINT_OK) {
        if (millis() - start > timeoutMs) return false;
        vTaskDelay( 100 / portTICK_PERIOD_MS);
    }
    return true;
}
