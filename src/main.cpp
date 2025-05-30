#define LOG_LEVEL_LOCAL ESP_LOG_VERBOSE
#define LOG_TAG "MAIN"

#include "main.h"
#include <esp_log.h>
#include <nvs_flash.h>

#include "enum/SystemState.h"
#include "enum/LockType.h"

#include "AdafruitFingerprintSensor.h"
#include "AdafruitNFCSensor.h"
#include "DoorRelay.h"
#include "repository/SDCardModule/SDCardModule.h"

#include "communication/ble/core/BLEModule.h"
#include "ota/ota.h"

#include "service/FingerprintService.h"
#include "service/NFCService.h"
#include "service/SyncService.h"
#include "service/WifiService.h"

#include "tasks/NFCTask/NFCTask.h"
#include "tasks/FingerprintTask/FingerprintTask.h"
#include "tasks/WifiTask/WifiTask.h"

#include "entity/CommandBleData.h"
#include "entity/QueueMessage.h"

// Initialize public state
static SystemState systemState = RUNNING;

// Initialize queues for sending message between fingerprint, nfc tasks to others
QueueHandle_t fingerprintQueueRequest;
QueueHandle_t fingerprintQueueResponse;
QueueHandle_t nfcQueueRequest;
QueueHandle_t nfcQueueResponse;

extern "C" void app_main(void){
    // Initialize the NVS Storage for Bluetooth and Wifi credentials
    // https://www.esp32.com/viewtopic.php?t=26365
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    // Initializing the Communication Service and Protocols
    BLEModule *bleModule = new BLEModule();
    OTA *otaModule = new OTA();

    // Setup BLE
    bleModule -> initBLE();
    bleModule -> setupCharacteristic();
    bleModule -> setupAdvertising();
    
    // Create a queue for handling WiFi States
    fingerprintQueueRequest = xQueueCreate(10, sizeof(FingerprintQueueRequest));
    fingerprintQueueResponse = xQueueCreate(10, sizeof(FingerprintQueueResponse));
    nfcQueueRequest = xQueueCreate(10, sizeof(NFCQueueRequest));
    nfcQueueResponse = xQueueCreate(10, sizeof(NFCQueueResponse));

    // Initialize the Sensor and Electrical Components
    SDCardModule *sdCardModule = new SDCardModule();
    FingerprintSensor *adafruitFingerprintSensor = new AdafruitFingerprintSensor();
    AdafruitNFCSensor *adafruitNFCSensor = new AdafruitNFCSensor();
    DoorRelay *doorRelay = new DoorRelay();

    // Initialize the Service
    FingerprintService *fingerprintService = new FingerprintService(adafruitFingerprintSensor, sdCardModule, doorRelay, bleModule, fingerprintQueueRequest, fingerprintQueueResponse);
    NFCService *nfcService = new NFCService(adafruitNFCSensor, sdCardModule, doorRelay, bleModule, nfcQueueRequest, nfcQueueResponse);
    SyncService *syncService = new SyncService(sdCardModule, bleModule);
    WifiService *wifiService = new WifiService(bleModule, otaModule, sdCardModule);

    // Initialize the Task
    NFCTask *nfcTask = new NFCTask("NFC Task", 3, nfcService);
    FingerprintTask *fingerprintTask = new FingerprintTask("Fingerprint Task", 3, fingerprintService);
    WifiTask *wifiTask = new WifiTask("Wifi Task", 10, wifiService, nfcQueueRequest, nfcQueueResponse, fingerprintQueueRequest, fingerprintQueueResponse);

    // Start Task
    nfcTask -> startTask();
    fingerprintTask -> startTask();
    wifiTask -> startTask();     // Setup Wifi Task

    // Checking the heap size after task init start task creation
    ESP_LOGI(LOG_TAG, "Heap Size Information!");
    ESP_LOGI(LOG_TAG, "Heap size: %u bytes", ESP.getHeapSize());
    ESP_LOGI(LOG_TAG, "Free heap: %u bytes", ESP.getFreeHeap());
    ESP_LOGI(LOG_TAG, "Minimum free heap ever: %u bytes", ESP.getMinFreeHeap());
     
    // Loop Main Mechanism
    while (1) {
        ESP_LOGD(LOG_TAG, "Running Main Thread");

        // Access current command data from extern static
        const char *command = commandBleData.getCommand();
        const char *name = commandBleData.getName();
        const char *visitorId = commandBleData.getVisitorId();
        const char *keyAccessId = commandBleData.getKeyAccess();

        switch (systemState) {
            case RUNNING:
                if (command != nullptr) {
                    if (strcmp(command, "register_fp") == 0) {
                        systemState = ENROLL_FP;
                    }
                    if (strcmp(command, "delete_fp") == 0) {
                        systemState = DELETE_FP;
                    }
                    if (strcmp(command, "delete_fp_user") == 0){
                        systemState = DELETE_FP_USER;
                    }
                    if (strcmp(command, "delete_fp_sensor") == 0){
                        systemState = DELETE_FP_SENSOR;
                    }
                    if (strcmp(command, "delete_fp_fs") == 0){
                        systemState = DELETE_FP_FS;
                    }
                    if (strcmp(command, "register_rfid") == 0) {
                        systemState = ENROLL_RFID;
                    }
                    if (strcmp(command, "delete_rfid") == 0) {
                        systemState = DELETE_RFID;
                    }
                    if (strcmp(command, "delete_rfid_user") == 0){
                        systemState = DELETE_RFID_USER;
                    }
                    if (strcmp(command, "delete_rfid_fs") == 0){
                        systemState = DELETE_RFID_FS;
                    }
                    if (strcmp(command, "update_visitor") == 0) {
                        systemState = UPDATE_VISITOR;
                    }
                    if (strcmp(command, "delete_access_user") == 0){
                        systemState = DELETE_ACCESS_USER;
                    }
                    if (strcmp(command, "door_lock") == 0){
                        systemState = DOOR_LOCK;
                    }
                    if (strcmp(command, "door_unlock") == 0){
                        systemState = DOOR_UNLOCK;
                    }
                }
                break;
            
            case ENROLL_RFID:
                ESP_LOGI(LOG_TAG,"Start Registering RFID!");
                nfcTask -> suspendTask();

                nfcService->addNFC(name, visitorId, keyAccessId);
                vTaskDelay(1000 / portTICK_PERIOD_MS);

                systemState = RUNNING;
                commandBleData.clear();
                nfcTask->resumeTask();
                break;

            case DELETE_RFID:
                ESP_LOGI(LOG_TAG, "Start Deleting RFID!");
                nfcTask->suspendTask();

                nfcService->deleteNFC(keyAccessId);
                vTaskDelay(1000 / portTICK_PERIOD_MS);

                systemState = RUNNING;
                commandBleData.clear();
                nfcTask->resumeTask();
                break;

            case DELETE_RFID_USER:
                ESP_LOGI(LOG_TAG, "Start Deleting User RFID!");
                nfcTask->suspendTask();

                nfcService->deleteNFCsUser(visitorId);
                vTaskDelay(1000 / portTICK_PERIOD_MS);

                systemState = RUNNING;
                commandBleData.clear();
                nfcTask->resumeTask();
                break;

            case DELETE_RFID_FS:
                ESP_LOGI(LOG_TAG, "Start Deleting RFID .json Key Access file!");
                nfcTask->suspendTask();

                nfcService->deleteNFCAccessFile();
                vTaskDelay(1000 / portTICK_PERIOD_MS);

                systemState = RUNNING;
                commandBleData.clear();
                nfcTask->resumeTask();
                break;

            case ENROLL_FP:
                ESP_LOGI(LOG_TAG, "Start Registering Fingerprint!");
                fingerprintTask->suspendTask();

                fingerprintService->addFingerprint(name, visitorId, keyAccessId);
                vTaskDelay(1000 / portTICK_PERIOD_MS);

                systemState = RUNNING;
                commandBleData.clear();
                fingerprintTask->resumeTask();
                break;

            case DELETE_FP:
                ESP_LOGI(LOG_TAG, "Start Deleting Fingerprint!");
                fingerprintTask->suspendTask();
                
                fingerprintService->deleteFingerprint(keyAccessId);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                
                systemState = RUNNING;
                commandBleData.clear();
                fingerprintTask->resumeTask();
                break;
                
            case DELETE_FP_USER:
                ESP_LOGI(LOG_TAG, "Start Deleting User Fingerprint!");
                fingerprintTask->suspendTask();

                fingerprintService->deleteFingerprintsUser(visitorId);
                vTaskDelay(1000 / portTICK_PERIOD_MS);

                systemState = RUNNING;
                commandBleData.clear();
                fingerprintTask->resumeTask();
                break;

            case DELETE_FP_FS:
                ESP_LOGI(LOG_TAG, "Start Deleting Fingerprint .json Key Access file!");
                fingerprintTask->suspendTask();

                fingerprintService->deleteFingerprintAccessFile();
                vTaskDelay(1000 / portTICK_PERIOD_MS);

                systemState = RUNNING;
                commandBleData.clear();
                fingerprintTask->resumeTask();
                break;

            case DELETE_FP_SENSOR:
                ESP_LOGI(LOG_TAG, "Start Deleting All Fingerprint Models!");
                fingerprintTask->suspendTask();

                fingerprintService->deleteAllFingerprintModel();
                vTaskDelay(1000 / portTICK_PERIOD_MS);

                systemState = RUNNING;
                commandBleData.clear();
                fingerprintTask->resumeTask();
                break;

            case DELETE_ACCESS_USER:
                // Exception to use the { } scoped block only if
                // You are to lazy to make this into one service, as it need multiple
                // colaboration between two or more service to be worked on
                {
                ESP_LOGI(LOG_TAG, "Start Deleting All Key Access Under User!");

                fingerprintTask->suspendTask();
                nfcTask->suspendTask();

                // Declare and initialize variables inside the block
                bool fingerprintUserDeleted = fingerprintService->deleteFingerprintsUser(visitorId);
                bool nfcUserDeleted = nfcService->deleteNFCsUser(visitorId);

                if (fingerprintUserDeleted)
                    ESP_LOGI(LOG_TAG, "Fingerprint key access successfully deleted for Visitor ID: %s", visitorId);
                else
                    ESP_LOGW(LOG_TAG, "Failed to delete fingerprint key access for Visitor ID: %s", visitorId);

                if (nfcUserDeleted)
                    ESP_LOGI(LOG_TAG, "NFC key access successfully deleted for Visitor ID: %s", visitorId);
                else
                    ESP_LOGW(LOG_TAG, "Failed to delete NFC key access for Visitor ID: %s", visitorId);

                // If at least one operation is successful, send success notification
                if (fingerprintUserDeleted || nfcUserDeleted) {
                    ESP_LOGI(LOG_TAG, "At least one key access entry was deleted successfully for Visitor ID: %s", visitorId);
                    bleModule->sendReport(SUCCESS_DELETE_USERS_KEY_ACCESS);
                }

                // If both operations failed, send failure notification
                if (!fingerprintUserDeleted && !nfcUserDeleted) {
                    ESP_LOGW(LOG_TAG, "Both key access entries failed to delete for Visitor ID: %s", visitorId);
                    bleModule->sendReport(FAILED_DELETE_USERS_KEY_ACCESS);
                }
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                
                systemState = RUNNING;
                commandBleData.clear();
                fingerprintTask->resumeTask();
                nfcTask->resumeTask();
                }
                break;

            case UPDATE_VISITOR:
                ESP_LOGI(LOG_TAG, "Start Sync Data!");
                fingerprintTask->suspendTask();
                nfcTask->suspendTask();

                syncService->sync();
                vTaskDelay(1000 / portTICK_PERIOD_MS);

                systemState = RUNNING;
                commandBleData.clear();
                fingerprintTask->resumeTask();
                nfcTask->resumeTask();
                break;
            
            case DOOR_LOCK:
                ESP_LOGI(LOG_TAG, "Closing the Door!");
                doorRelay->lockRelay();

                vTaskDelay(100 / portTICK_PERIOD_MS);

                systemState = RUNNING;
                commandBleData.clear();
                break;

            case DOOR_UNLOCK:
                ESP_LOGI(LOG_TAG, "Opening the Door!");
                doorRelay->unlockRelay();

                vTaskDelay(100 / portTICK_PERIOD_MS);

                systemState = RUNNING;
                commandBleData.clear();
                break;

            default:
                break;
        }

        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}
