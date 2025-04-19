#ifndef WIFI_TASK_H
#define WIFI_TASK_H

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <esp_log.h>

#include "service/WifiService.h"

#include "tasks/BaseTask.h"
#include "communication/wifi/Wifi.h"

#include "enum/OperationState.h"
#include "entity/QueueMessage.h"

/// @brief Class for managing the WiFi Task Action
class WifiTask : BaseTask {
    public:
        WifiTask(const char* taskName, UBaseType_t priority, WifiService *wifiService, 
                 QueueHandle_t nfcQueueRequest, QueueHandle_t nfcQueueResponse, 
                 QueueHandle_t fingerprintQueueRequest, QueueHandle_t fingerprintQueueResponse);
        ~WifiTask();
        void startTask() override;
        bool suspendTask() override;
        bool resumeTask() override;
        void handleNFCTask(NFCQueueRequest message);
        void handleFingerprintTask(FingerprintQueueRequest message);
        
    private:
        const char* _taskName;
        UBaseType_t _priority;
        TaskHandle_t _taskHandle;
        SemaphoreHandle_t _xWifiSemaphore;
        WifiService* _wifiService;
        QueueHandle_t _nfcQueueRequest;
        QueueHandle_t _nfcQueueResponse;
        QueueHandle_t _fingerprintQueueRequest;
        QueueHandle_t _fingerprintQueueResponse;
        static void loop(void *parameter);
        static void reconnect(void *parameter);
};

#endif
