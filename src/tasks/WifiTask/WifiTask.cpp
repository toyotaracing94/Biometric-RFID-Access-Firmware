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

    // Spawning job schedule to periodically check wether WiFi is connected or not
    // If yes, then that means there something wrong with the source
    // Lets just create new connection
    xTaskCreate(
        reconnect,                  // Function to run in the task
        "reconnect",                // Name of the task
        MINIMUM_STACK_SIZE,         // Stack size (adjustable), I'll set this to maximum personal config just in case
        task,                       // Pass the `this` pointer to the task
        5,                          // Task priority
        NULL                        // Store the task handle for later control
    );
    ESP_LOGI(WIFI_TASK_LOG_TAG, "Wifi Task Job Schedule for checking WiFi Connectivity created successfully: Task Name = %s, Priority = %d", "reconnect", 5);

    // Spawning job schedule to periodically check/listen for any OTA updates request from external
    xTaskCreate(
        listenOTA,                  // Function to run in the task
        "listenOTA",                // Name of the task
        MINIMUM_STACK_SIZE,         // Stack size (adjustable), I'll set this to maximum personal config just in case
        task,                       // Pass the `this` pointer to the task
        5,                          // Task priority
        NULL                        // Store the task handle for later control
    );
    ESP_LOGI(WIFI_TASK_LOG_TAG, "Wifi Task Job Schedule for listening OTA updates created successfully: Task Name = %s, Priority = %d", "listenOTA", 5);

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
 * @brief Scheduler Job FreeRTOS loop for the WifiTask.
 *
 * Check the WiFi does the WiFi is connected to any, if not then spawn new Config portal to change the
 * connection to others WiFi
 *
 * @param params Pointer to the WifiTask instance (cast from void*).
 */
void WifiTask::reconnect(void *params){
    WifiTask* task = (WifiTask*)params;

    while (1){
        // Start reconnecting thread job
        ESP_LOGI(WIFI_TASK_LOG_TAG, "Start Reconnect Job Schedule!!");
        bool reconnected = task-> _wifiService->reconnect();

        if(reconnected) ESP_LOGI(WIFI_TASK_LOG_TAG, "Success reconnected!");
        else ESP_LOGE(WIFI_TASK_LOG_TAG, "Failed to reconnect. Possibly not needed or a timeout");
        
        // Will start this job in every 3 minutes to check, it's because the timeout set to 3 minutes for the portal
        // So adjusting that
        vTaskDelay(3 * 60 * 1000 / portTICK_PERIOD_MS);
    }
}

/**
 * @brief Scheduler Job FreeRTOS loop for the WifiTask OTA Service.
 * Run in loop for listening for any OTA request from external
 *  
 * @param params Pointer to the WifiTask instance (cast from void*).
 */
void WifiTask::listenOTA(void *params){
    WifiTask* task = (WifiTask*)params;
    ESP_LOGI(WIFI_TASK_LOG_TAG, "Start OTA Job Schedule!!");

    // Start beginning the OTA Service
    ESP_LOGI(WIFI_TASK_LOG_TAG, "Start OTA Service");
    task->_wifiService->beginOTA();

    // Checking the heap size after task creation
    ESP_LOGI(WIFI_TASK_LOG_TAG, "Heap Size Information After OTA Service Begin!");
    ESP_LOGI(WIFI_TASK_LOG_TAG, "Heap size: %u bytes", ESP.getHeapSize());
    ESP_LOGI(WIFI_TASK_LOG_TAG, "Free heap: %u bytes", ESP.getFreeHeap());
    ESP_LOGI(WIFI_TASK_LOG_TAG, "Minimum free heap ever: %u bytes", ESP.getMinFreeHeap());

    vTaskDelay(50 / portTICK_PERIOD_MS); // Just caution to give enough time for the watchdog to process this OTA beginning

    while (1){
        ESP_LOGD(WIFI_TASK_LOG_TAG, "Start OTA Listening Service");
        task->_wifiService->handleOTA();
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

    if (message.state == ENROLL_RFID) {
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
    
    if (message.state == DELETE_RFID) {
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
    
    if (message.state == AUTHENTICATE_RFID) {
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
    
    if (message.state == ENROLL_FP) {
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
    
    if (message.state == DELETE_FP) { 
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
    
    if (message.state == AUTHENTICATE_FP) {
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

