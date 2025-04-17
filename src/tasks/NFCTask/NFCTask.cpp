#include "NFCTask.h"
#define NFC_TASK_LOG_TAG "NFC_TASK"

NFCTask::NFCTask(const char* taskName, UBaseType_t priority, NFCService* nfcService)
    : _taskName(taskName), _priority(priority), _nfcService(nfcService) {
        
        _xNfcSemaphore = xSemaphoreCreateBinary();\
        if (_xNfcSemaphore == NULL) ESP_LOGE(NFC_TASK_LOG_TAG, "Failed to create NFC semaphore.");
        xSemaphoreGive(_xNfcSemaphore);
    }

/**
 * @brief Create the NFC Task.
 * 
 */
void NFCTask::startTask() {
    xTaskCreate(
        taskFunction,               // Function to run in the task
        _taskName,                  // Name of the task
        MIDSIZE_STACK_SIZE,         // Stack size (adjustable)
        this,                       // Pass the `this` pointer to the task
        _priority,                  // Task priority
        &_taskHandle                // Store the task handle for later control
    );
    ESP_LOGI(NFC_TASK_LOG_TAG, "NFC Task created successfully: Task Name = %s, Priority = %d", _taskName, _priority);
}

/**
 * @brief Suspend the NFC Task operation.
 * 
 */
bool NFCTask::suspendTask(){
    if (_taskHandle != nullptr) {
        if (xSemaphoreTake(_xNfcSemaphore, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(NFC_TASK_LOG_TAG, "NFC Task is suspended.");
            return true;
        }
        ESP_LOGW(NFC_TASK_LOG_TAG, "NFC Task is unable to be suspended.");
        return false;
    } else {
        ESP_LOGE(NFC_TASK_LOG_TAG, "NFC Task handle is null.");
        return false;
    }
}

/**
 * @brief Resume the NFC Task operation.
 * 
 */
bool NFCTask::resumeTask(){
    if (_taskHandle != nullptr) {
        xSemaphoreGive(_xNfcSemaphore);
        ESP_LOGI(NFC_TASK_LOG_TAG, "NFC Task is resumed.");
        return true;
    } else {
        ESP_LOGE(NFC_TASK_LOG_TAG, "NFC Task handle is null.");
        return false;
    }
}

/**
 * @brief Function to be run by the NFC Task.
 * 
 * This function is the main task routine that continuously authenticates access 
 * using the NFC sensor.
 * 
 * @param params Pointer to the task parameters (in this case, the NFCTask instance).
 * @return void
 */
void NFCTask::taskFunction(void *params){
    NFCTask* task = (NFCTask*)params;

    while(1){
        ESP_LOGD(NFC_TASK_LOG_TAG, "Running Routine NFC Thread");

        if (xSemaphoreTake( task-> _xNfcSemaphore, portMAX_DELAY) == pdTRUE) {
            if (task-> _nfcService->authenticateAccessNFC()) 
                ESP_LOGD(NFC_TASK_LOG_TAG, "NFC access granted.");
            else 
                ESP_LOGD(NFC_TASK_LOG_TAG, "NFC access denied.");
            xSemaphoreGive(task-> _xNfcSemaphore);
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

    