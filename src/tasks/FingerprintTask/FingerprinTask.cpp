#include "FingerprintTask.h"
#define FINGERPRINT_TASK_LOG_TAG "FINGERPRINT_TASK"

FingerprintTask::FingerprintTask(const char* taskName, UBaseType_t priority, TaskHandle_t* taskHandle, FingerprintService *fingerprintService)
    : _taskName(taskName), _priority(priority),  _taskHandle(taskHandle), _fingerprintService(fingerprintService) {}

void FingerprintTask::createTask() {
    xTaskCreate(taskFunction, _taskName, TASK_STACK_SIZE, this, _priority, &_taskHandle);
    ESP_LOGI(FINGERPRINT_TASK_LOG_TAG, "Fingerprint Task created successfully: Task Name = %s, Priority = %d", _taskName, _priority);
}

void FingerprintTask::suspendTask(){
    if (_taskHandle != nullptr) {
        vTaskSuspend(_taskHandle);
        ESP_LOGI(FINGERPRINT_TASK_LOG_TAG, "Fingerprint Task is suspended.");
    } else {
        ESP_LOGE(FINGERPRINT_TASK_LOG_TAG, "Fingerprint Task handle is null.");
    }
}

void FingerprintTask::resumeTask(){
    if (_taskHandle != nullptr) {
        vTaskResume(_taskHandle);
        ESP_LOGI(FINGERPRINT_TASK_LOG_TAG, "Fingerprint Task resumed.");
    } else {
        ESP_LOGE(FINGERPRINT_TASK_LOG_TAG, "Fingerprint Task handle is null.");
    }
}

void FingerprintTask::taskFunction(void *params){
    // TODO: Search like what the hell is this
    // Embarasing to say, this is what I envision for separating the Task Service
    // But this came from chatGPT and without filtering this first
    // also how can I implememt Signal and Queue for now if I wrap them inside class?
    FingerprintTask* fingerprintTask = static_cast<FingerprintTask*>(params);
    while(1){
        ESP_LOGD(FINGERPRINT_TASK_LOG_TAG, "Running Routine Fingerprint Thread");
        fingerprintTask->_fingerprintService->authenticateAccessFingerprint();
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
