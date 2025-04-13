#include "WifiTask.h"
#define WIFI_TASK_LOG_TAG "WIFI_TASK"

WifiTask::WifiTask(const char* taskName, UBaseType_t priority, QueueHandle_t nfcToWiFiQueue, QueueHandle_t fingerprintToWiFiQueue)
    : _taskName(taskName), _priority(priority) {
        
        // Referencing the queues message
        _nfcToWiFiQueue = nfcToWiFiQueue;
        _fingerprintToWiFiQueue = fingerprintToWiFiQueue;

        _xWifiSemaphore = xSemaphoreCreateBinary();
        xSemaphoreGive(_xWifiSemaphore);

        // Create new object of Wifi for the Wifi Tasks
        _wifi = new Wifi();
    }

/**
 * @brief Create the Wifi Task.
 * 
 */
void WifiTask::startTask() {
    xTaskCreatePinnedToCore(
        loop,                       // Function to run in the task
        _taskName,                  // Name of the task
        MAX_STACK_SIZE,             // Stack size (adjustable), I'll set this to maximum personal config just in case
        this,                       // Pass the `this` pointer to the task
        _priority,                  // Task priority
        &_taskHandle,               // Store the task handle for later control,
        1
    );
    ESP_LOGI(WIFI_TASK_LOG_TAG, "Wifi Task created successfully: Task Name = %s, Priority = %d", _taskName, _priority);
}

/**
 * @brief Suspend the Wifi Task operation.
 * 
 */
bool WifiTask::suspendTask(){
    if (_taskHandle != nullptr) {
        if (xSemaphoreTake(_xWifiSemaphore, 0) == pdTRUE) {
            ESP_LOGI(WIFI_TASK_LOG_TAG, "WiFi Task is suspended.");
            return true;
        }
        ESP_LOGW(WIFI_TASK_LOG_TAG, "WiFi Task is unable to be suspended.");
        return false;
    } else {
        ESP_LOGE(WIFI_TASK_LOG_TAG, "WiFi Task handle is null.");
        return false;
    }
}

/**
 * @brief Resume the Wifi Task operation.
 * 
 */
bool WifiTask::resumeTask(){
    if (_taskHandle != nullptr) {
        xSemaphoreGive(_xWifiSemaphore);
        ESP_LOGI(WIFI_TASK_LOG_TAG, "WiFi Task is resumed.");
        return true;
    } else {
        ESP_LOGE(WIFI_TASK_LOG_TAG, "WiFi Task handle is null.");
        return false;
    }
}

void WifiTask::loop(void *params){
    WifiTask* task = (WifiTask*)params;
    
    // Initialize the Wifi
    task -> _wifi -> init(); 

    // Hold the queues message
    nfcQueueMessage nfcMessage;
    fingerprintQueueMessage fingerprintMessage;

    while(1){
        ESP_LOGD(WIFI_TASK_LOG_TAG, "Running Routine Wifi Thread | Reading queue for incoming data");
        
        if (xQueueReceive( task-> _nfcToWiFiQueue, &nfcMessage, 0) == pdTRUE) {
            if( task-> _wifi -> isConnected()){
                ESP_LOGD(WIFI_TASK_LOG_TAG, "Running Routine Wifi Thread | Reading queue for incoming data");
            }
        }
        
        if (xQueueReceive( task-> _fingerprintToWiFiQueue, &fingerprintMessage, 0) == pdTRUE) {
            if( task-> _wifi -> isConnected()){
                ESP_LOGD(WIFI_TASK_LOG_TAG, "Running Routine Wifi Thread | Reading queue for incoming data");
            }
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void WifiTask::handleNFCTask(nfcQueueMessage message) {
    if (message.operation == ENROLL_RFID) {
        sendToServer("/api/nfc/add", message);
    } 
    
    if (message.operation == DELETE_RFID) {
        sendToServer("/api/nfc/delete", message);
    }
}

void WifiTask::handleFingerprintTask(fingerprintQueueMessage message) {
    if (message.operation == ENROLL_FP) {
        sendToServer("/api/nfc/add", message);
    } 
    
    if (message.operation == DELETE_FP) {
        sendToServer("/api/nfc/delete", message);
    }
}

void WifiTask::sendToServer(String route, nfcQueueMessage message){
    // TODO: LOGIC FOR SENDING TO SERVER
    ESP_LOGI(WIFI_TASK_LOG_TAG, "Processing NFC Task to WiFi");
}

void WifiTask::sendToServer(String route, fingerprintQueueMessage message){
    // TODO: LOGIF FOR SENDING TO SERVER
    ESP_LOGI(WIFI_TASK_LOG_TAG, "Processing Fingerprint Task to WiFi");
}


