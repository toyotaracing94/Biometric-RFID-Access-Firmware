#ifndef FINGERPRINT_TASK_H
#define FINGERPRINT_TASK_H

#define LOG_TAG "FINGERPRINT_TASK"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#include "tasks/BaseTask.h"
#include "service/FingerprintService.h"
#define TASK_STACK_SIZE 4096

class FingerprintTask : BaseTask {
    public:
        FingerprintTask(const char* taskName, UBaseType_t priority, TaskHandle_t* taskHandle,  FingerprintService *fingerprintService);
        ~FingerprintTask();
        void createTask() override;
        void suspendTask() override;
        void resumeTask() override;
    
    private:
        const char* _taskName;
        UBaseType_t _priority;
        TaskHandle_t _taskHandle; 
        FingerprintService* _fingerprintService;
        static void taskFunction(void *parameter);
};


#endif