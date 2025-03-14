#ifndef NFC_TASK_H
#define NFC_TASK_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#include "tasks/BaseTask.h"

#include "service/NFCService.h"
#define TASK_STACK_SIZE 4096

/// @brief Class for managing the NFC Task Action
class NFCTask : BaseTask {
    public:
        NFCTask(const char* taskName, UBaseType_t priority, TaskHandle_t* taskHandle,  NFCService *nfcService);
        ~NFCTask();
        void createTask() override;
        void suspendTask() override;
        void resumeTask() override;
        
    private:
        const char* _taskName;
        UBaseType_t _priority;
        TaskHandle_t _taskHandle; 
        NFCService* _nfcService;
        static void taskFunction(void *parameter);
};

#endif
