#ifndef NFC_TASK_H
#define NFC_TASK_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <esp_log.h>

#include "tasks/BaseTask.h"

#include "service/NFCService.h"

/// @brief Class for managing the NFC Task Action
class NFCTask : BaseTask {
    public:
        NFCTask(const char* taskName, UBaseType_t priority, NFCService *nfcService);
        ~NFCTask();
        void startTask() override;
        bool suspendTask() override;
        bool resumeTask() override;
        
    private:
        const char* _taskName;
        UBaseType_t _priority;
        TaskHandle_t _taskHandle; 
        SemaphoreHandle_t _xNfcSemaphore;
        NFCService* _nfcService;
        static void taskFunction(void *parameter);
};

#endif
