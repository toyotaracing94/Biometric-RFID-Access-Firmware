#include "FingerprintTask.h"
#define FINGERPRINT_TASK_LOG_TAG "FINGERPRINT_TASK"

FingerprintTask::FingerprintTask(const char* taskName, UBaseType_t priority, FingerprintService *fingerprintService)
    : _taskName(taskName), _priority(priority), _fingerprintService(fingerprintService) {
        
        _xFingerprintSemaphore = xSemaphoreCreateBinary();
        xSemaphoreGive(_xFingerprintSemaphore);
    }

/**
 * @brief Create the Fingerprint Task.
 * 
 */
void FingerprintTask::startTask() {
    xTaskCreate(
        taskFunction,               // Function to run in the task
        _taskName,                  // Name of the task
        MIDSIZE_STACK_SIZE,         // Stack size (adjustable)
        this,                       // Pass the `this` pointer to the task
        _priority,                  // Task priority
        &_taskHandle                // Store the task handle for later control
    );
    ESP_LOGI(FINGERPRINT_TASK_LOG_TAG, "Fingerprint Task created successfully: Task Name = %s, Priority = %d", _taskName, _priority);
}

/**
 * @brief Suspend the Fingerprint Task operation.
 * 
 */
bool FingerprintTask::suspendTask(){
    if (_taskHandle != nullptr){
        if (xSemaphoreTake(_xFingerprintSemaphore, 0) == pdTRUE) {
            ESP_LOGI(FINGERPRINT_TASK_LOG_TAG, "Fingerprint Task is suspended.");
            return true;
        }
        ESP_LOGW(FINGERPRINT_TASK_LOG_TAG, "Fingerprint Task is unable to be suspended.");
        return false;
    }else{
        ESP_LOGE(FINGERPRINT_TASK_LOG_TAG, "Fingerprint Task handle is null.");
        return false;
    }
}

/**
 * @brief Resume the Fingerprint Task operation.
 * 
 */
bool FingerprintTask::resumeTask(){
    if (_taskHandle != nullptr) {
        xSemaphoreGive(_xFingerprintSemaphore);
        ESP_LOGI(FINGERPRINT_TASK_LOG_TAG, "Fingerprint Task resumed.");
        return true;
    } else {
        ESP_LOGE(FINGERPRINT_TASK_LOG_TAG, "Fingerprint Task handle is null.");
        return false;
    }
}

/**
 * @brief Function to be run by the Fingerprint Task.
 * 
 * This function is the main task routine that continuously authenticates access 
 * using the fingerprint sensor.
 * 
 * @param params Pointer to the task parameters (in this case, the FingerprintTask instance).
 */
void FingerprintTask::taskFunction(void *params){
    // TODO: Search like what the hell is this
    // Embarasing to say, this is what I envision for separating the Task Service
    // But this came from chatGPT and without filtering this first
    // also how can I implememt Signal and Queue for now if I wrap them inside class?
    FingerprintTask* task = (FingerprintTask*)params;

    while(1){
        ESP_LOGD(FINGERPRINT_TASK_LOG_TAG, "Running Routine Fingerprint Thread");

        if (xSemaphoreTake( task-> _xFingerprintSemaphore, portMAX_DELAY) == pdTRUE) {
            if (task-> _fingerprintService -> authenticateAccessFingerprint()) 
                ESP_LOGD(FINGERPRINT_TASK_LOG_TAG, "Fingerprint access granted.");
            else 
                ESP_LOGD(FINGERPRINT_TASK_LOG_TAG, "Fingerprint access denied.");
            xSemaphoreGive(task-> _xFingerprintSemaphore);
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
