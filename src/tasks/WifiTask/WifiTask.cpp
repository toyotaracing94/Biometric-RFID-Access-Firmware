#include "WifiTask.h"
#define WIFI_TASK_LOG_TAG "WIFI_TASK"

WifiTask::WifiTask(const char* taskName, UBaseType_t priority, WifiService* wifiService, QueueHandle_t nfcQueueRequest, QueueHandle_t nfcQueueResponse, QueueHandle_t fingerprintQueueRequest, QueueHandle_t fingerprintQueueResponse)
    : _taskName(taskName), _priority(priority), _wifiService(wifiService) {
        
        // Referencing the queues message
        _nfcQueueRequest = nfcQueueRequest;
        _nfcQueueResponse = nfcQueueResponse;
        _fingerprintQueueRequest = fingerprintQueueRequest;
        _fingerprintQueueResponse = fingerprintQueueResponse;

        _xWifiSemaphore = xSemaphoreCreateBinary();
        xSemaphoreGive(_xWifiSemaphore);
    }

/**
 * @brief Create the Wifi Task.
 * 
 * @note Previously I always got error of
 * [E][WiFiClient.cpp:395] write(): fail on fd 54, errno: 11, "No more processes"
 * when wrapping this WiFi manager to this WiFi Task. I concure it's from the memory consumption as when I start the service
 * from main thread, there no wrong what's so ever. I fix this by reducing the stack size for the WiFi task from MAXIMUM_STACK_SIZE to MIDSIZE_STACK_SIZE
 * 
 * Here's the link for the some answer possibly https://github.com/espressif/arduino-esp32/issues/6129#issuecomment-2490883110
 */
void WifiTask::startTask() {
    xTaskCreate(
        loop,                       // Function to run in the task
        _taskName,                  // Name of the task
        MIDSIZE_STACK_SIZE,         // Stack size (adjustable), I'll set this to maximum personal config just in case
        this,                       // Pass the `this` pointer to the task
        _priority,                  // Task priority
        &_taskHandle                // Store the task handle for later control
    );
    ESP_LOGI(WIFI_TASK_LOG_TAG, "Wifi Task created successfully: Task Name = %s, Priority = %d", _taskName, _priority);
}

/**
 * @brief Suspend the Wifi Task operation.
 * 
 */
bool WifiTask::suspendTask() {
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

/**
 * @brief Main FreeRTOS loop for the WifiTask.
 *
 * Initializes the WiFi service and continuously listens for NFC and fingerprint
 * requests from their respective queues.
 *
 * @param params Pointer to the WifiTask instance (cast from void*).
 */
void WifiTask::loop(void *params){
    WifiTask* task = (WifiTask*)params;
    
    // Initialize the Wifi operation inside the task
    vTaskDelay(1000 / portTICK_PERIOD_MS); // Give time for system to catch up
    task -> _wifiService -> setup(); 

    // Hold the queues message
    NFCQueueRequest nfcMessage;
    FingerprintQueueRequest fingerprintMessage;

    while(1){
        ESP_LOGD(WIFI_TASK_LOG_TAG, "Running Routine Wifi Thread | Reading queue for incoming data");
        
        // Check wether there's new queue message in every 100ms
        if (xQueueReceive( task-> _nfcQueueRequest, &nfcMessage, 100) == pdTRUE) {
            ESP_LOGD(WIFI_TASK_LOG_TAG, "Running Routine Wifi Thread | Reading queue for incoming data");
            task->handleNFCTask(nfcMessage);
        }
        
        // Check wether there's new queue message in every 100ms
        if (xQueueReceive( task-> _fingerprintQueueRequest, &fingerprintMessage, 100) == pdTRUE) {
            ESP_LOGD(WIFI_TASK_LOG_TAG, "Running Routine Wifi Thread | Reading queue for incoming data");
            task->handleFingerprintTask(fingerprintMessage);
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

/**
 * @brief The handler function when receviving queue from `_nfcQueueRequest` to be send from WiFi Service into back NFC Service
 * 
 * Processes NFC operations based on the request state, such as adding,
 * removing, or authenticating NFC (RFID) tags by communicating with the server.
 * 
 * @param message NFCQueueRequest message format from the queue
 */
void WifiTask::handleNFCTask(NFCQueueRequest message) {
    ESP_LOGI(WIFI_TASK_LOG_TAG, "Handling NFC task, state: %d", message.state);

    if (message.state == ADD_RFID) {
        ESP_LOGI(WIFI_TASK_LOG_TAG, "Adding NFC (RFID) data to server.");

        if(_wifiService->isConnected()){
            // Send the data to the server
            NFCQueueResponse response = _wifiService->addNFCToServer(message);
            ESP_LOGI(WIFI_TASK_LOG_TAG, "ADD_RFID response received.");
    
            // Send the result back to NFCService
            if (xQueueSend(_nfcQueueResponse, &response, portMAX_DELAY)) {
                ESP_LOGI(WIFI_TASK_LOG_TAG, "Sent ADD_RFID response back to NFCService.");
            } else {
                ESP_LOGE(WIFI_TASK_LOG_TAG, "Failed to send ADD_RFID response back to NFCService.");
            }
        }else{
            ESP_LOGW(WIFI_TASK_LOG_TAG, "Device is not connected to WiFi/Internet, so cannot proceed to do operation");
        }
        
    }
    
    if (message.state == REMOVE_RFID) {
        ESP_LOGI(WIFI_TASK_LOG_TAG, "Removing NFC (RFID) data from server.");
        
        if(_wifiService->isConnected()){
            // Send the data to the server
            NFCQueueResponse response = _wifiService->deleteNFCFromServer(message);
            
            ESP_LOGI(WIFI_TASK_LOG_TAG, "REMOVE_RFID response received.");
            
            // Send the result back to NFCService
            if (xQueueSend(_nfcQueueResponse, &response, portMAX_DELAY)) {
                ESP_LOGI(WIFI_TASK_LOG_TAG, "Sent REMOVE_RFID response back to NFCService.");
            } else {
                ESP_LOGE(WIFI_TASK_LOG_TAG, "Failed to send REMOVE_RFID response back to NFCService.");
            }
        } else{
            ESP_LOGW(WIFI_TASK_LOG_TAG, "Device is not connected to WiFi/Internet, so cannot proceed to do operation");
        }
    }
    
    if (message.state == AUTH_RFID) {
        ESP_LOGI(WIFI_TASK_LOG_TAG, "Authenticating NFC (RFID) access.");
        
        if(_wifiService->isConnected()){
            // Authenticate NFC access
            _wifiService->authenticateAccessNFCToServer(message);
            ESP_LOGI(WIFI_TASK_LOG_TAG, "NFC (RFID) access authentication request sent.");    
        }else{
            ESP_LOGW(WIFI_TASK_LOG_TAG, "Device is not connected to WiFi/Internet, will hold the data in temporary local history data access");
        }
    }
}

/**
 * @brief The handler function when receviving queue from `_fingerprintQueueRequest` to be send from WiFi Service into back Fingerprint Service
 * 
 * Processes Fingerprint operations based on the request state, such as adding,
 * removing, or authenticating Fingerprint by communicating with the server.
 * 
 * @param message FingerprintQueueRequest message format from the queue
 */
void WifiTask::handleFingerprintTask(FingerprintQueueRequest message) {
    ESP_LOGI(WIFI_TASK_LOG_TAG, "Handling Fingerprint task, state: %d", message.state);
    
    if (message.state == ADD_FP) {
        ESP_LOGI(WIFI_TASK_LOG_TAG, "Adding Fingerprint data to server.");
        
        
        if(_wifiService->isConnected()){
            // Send the data to the server
            FingerprintQueueResponse response = _wifiService->addFingerprintToServer(message);
            ESP_LOGI(WIFI_TASK_LOG_TAG, "ADD_FP response received.");
            
            // Send the result back to FingerprintService
            if (xQueueSend(_fingerprintQueueResponse, &response, portMAX_DELAY)) {
                ESP_LOGI(WIFI_TASK_LOG_TAG, "Sent ADD_FP response back to FingerprintService.");
            } else {
                ESP_LOGE(WIFI_TASK_LOG_TAG, "Failed to send ADD_FP response back to FingerprintService.");
            }
        }else{
            ESP_LOGW(WIFI_TASK_LOG_TAG, "Device is not connected to WiFi/Internet, so cannot proceed to do operation");
        }
    }
    
    if (message.state == REMOVE_FP) { 
        ESP_LOGI(WIFI_TASK_LOG_TAG, "Removing Fingerprint data from server.");
        
        if(_wifiService->isConnected()){
            // Send the data to the server
            FingerprintQueueResponse response = _wifiService->deleteFingerprintFromServer(message);
            
            ESP_LOGI(WIFI_TASK_LOG_TAG, "REMOVE_FP response received.");
            
            // Send the result back to FingerprintService
            if (xQueueSend(_fingerprintQueueResponse, &response, portMAX_DELAY)) {
                ESP_LOGI(WIFI_TASK_LOG_TAG, "Sent REMOVE_FP response back to FingerprintService.");
            } else {
                ESP_LOGE(WIFI_TASK_LOG_TAG, "Failed to send REMOVE_FP response back to FingerprintService.");
            }
        }else{
            ESP_LOGW(WIFI_TASK_LOG_TAG, "Device is not connected to WiFi/Internet, so cannot proceed to do operation");
        }
    }
    
    if (message.state == AUTH_FP) {
        ESP_LOGI(WIFI_TASK_LOG_TAG, "Authenticating Fingerprint access.");
        
        if(_wifiService->isConnected()){
            // Authenticate Fingerprint access
            _wifiService->authenticateAccessFingerprintToServer(message);
            ESP_LOGI(WIFI_TASK_LOG_TAG, "Fingerprint access authentication request sent.");
        }else{
            ESP_LOGW(WIFI_TASK_LOG_TAG, "Device is not connected to WiFi/Internet, will hold the data in temporary local history data access");
        }
    }
}

