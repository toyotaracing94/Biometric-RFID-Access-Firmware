#ifndef WIFI_TASK_H
#define WIFI_TASK_H

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <esp_log.h>

#include "tasks/BaseTask.h"
#include "communication/wifi/Wifi.h"

#include "enum/SystemState.h"
#include "entity/QueueMessage.h"

/// @brief Class for managing the WiFi Task Action
class WifiTask : BaseTask {
    public:
        WifiTask(const char* taskName, UBaseType_t priority, QueueHandle_t nfcToWiFiQueue, QueueHandle_t fingerprintToWiFiQueue);
        ~WifiTask();
        void startTask() override;
        bool suspendTask() override;
        bool resumeTask() override;
        void handleNFCTask(nfcQueueMessage message);
        void handleFingerprintTask(fingerprintQueueMessage message);
        void sendToServer(String route, nfcQueueMessage message);
        void sendToServer(String route, fingerprintQueueMessage message);
        
    private:
        const char* _taskName;
        UBaseType_t _priority;
        TaskHandle_t _taskHandle;
        SemaphoreHandle_t _xWifiSemaphore;
        QueueHandle_t _nfcToWiFiQueue;
        QueueHandle_t _fingerprintToWiFiQueue;
        Wifi* _wifi;
        static void loop(void *parameter);
};

#endif
