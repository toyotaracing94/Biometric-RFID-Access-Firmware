#include "NFCTask.h"
#define NFC_TASK_LOG_TAG "NFC_TASK"

NFCTask::NFCTask(const char* taskName, UBaseType_t priority, TaskHandle_t* taskHandle, NFCService* nfcService)
    : _taskName(taskName), _priority(priority),  _taskHandle(taskHandle), _nfcService(nfcService) {}

/**
 * @brief Create the NFC Task.
 * 
 */
void NFCTask::createTask() {
    xTaskCreate(taskFunction, _taskName, TASK_STACK_SIZE, this, _priority, &_taskHandle);
    ESP_LOGI(NFC_TASK_LOG_TAG, "NFC Task created successfully: Task Name = %s, Priority = %d", _taskName, _priority);
}

/**
 * @brief Suspend the NFC Task operation.
 * 
 */
void NFCTask::suspendTask(){
    if (_taskHandle != nullptr) {
        vTaskSuspend(_taskHandle);
        ESP_LOGI(NFC_TASK_LOG_TAG, "NFC Task is suspended.");
    } else {
        ESP_LOGE(NFC_TASK_LOG_TAG, "NFC Task handle is null.");
    }
}

/**
 * @brief Resume the NFC Task operation.
 * 
 */
void NFCTask::resumeTask(){
    if (_taskHandle != nullptr) {
        vTaskResume(_taskHandle);
        ESP_LOGI(NFC_TASK_LOG_TAG, "NFC Task is resumed.");
    } else {
        ESP_LOGE(NFC_TASK_LOG_TAG, "NFC Task handle is null.");
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
    NFCTask* nfcTask = static_cast<NFCTask*>(params);
    while(1){
        ESP_LOGD(NFC_TASK_LOG_TAG, "Running Routine Task Thread");
        nfcTask->_nfcService->authenticateAccessNFC();
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

    