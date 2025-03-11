#include "NFCTask.h"

NFCTask::NFCTask(const char* taskName, UBaseType_t priority, TaskHandle_t* taskHandle, NFCService* nfcService)
    : _taskName(taskName), _priority(priority),  _taskHandle(taskHandle), _nfcService(nfcService) {}

void NFCTask::createTask() {
    xTaskCreate(taskFunction, _taskName, TASK_STACK_SIZE , this, _priority, &_taskHandle);
}

void NFCTask::suspendTask(){
    if (_taskHandle != nullptr) {
        vTaskSuspend(_taskHandle);
        ESP_LOGI(LOG_TAG, "Task suspended.");
    } else {
        ESP_LOGE(LOG_TAG, "Task handle is null.");
    }
}

void NFCTask::resumeTask(){
    if (_taskHandle != nullptr) {
        vTaskResume(_taskHandle);
        ESP_LOGI(LOG_TAG, "Task resumed.");
    } else {
        ESP_LOGE(LOG_TAG, "Task handle is null.");
    }
}

void NFCTask::taskFunction(void *params){
    NFCTask* nfcTask = static_cast<NFCTask*>(params);
    while(1){
        ESP_LOGI(LOG_TAG, "Runnning NFC Task");
        vTaskDelay(200/ portTICK_PERIOD_MS);
        ESP_LOGI(LOG_TAG, "Reading and Authenticate RFID Card");
        nfcTask->_nfcService->authenticateAccessNFC();
    }
}

    