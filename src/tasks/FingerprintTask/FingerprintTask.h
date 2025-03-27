#ifndef FINGERPRINT_TASK_H
#define FINGERPRINT_TASK_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#include "tasks/BaseTask.h"
#include "service/FingerprintService.h"

/// @brief Class for managing the Fingerprint Task Action
class FingerprintTask : BaseTask {
    public:
        FingerprintTask(const char* taskName, UBaseType_t priority,  FingerprintService *fingerprintService);
        ~FingerprintTask();
        void startTask() override;
        bool suspendTask() override;
        bool resumeTask() override;
    
    private:
        const char* _taskName;
        UBaseType_t _priority;
        TaskHandle_t _taskHandle;                
        SemaphoreHandle_t _xFingerprintSemaphore;
        FingerprintService* _fingerprintService;
        static void taskFunction(void *parameter);
};


#endif