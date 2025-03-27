#define LOG_LEVEL_LOCAL ESP_LOG_VERBOSE
#define LOG_TAG "MAIN"

#include "main.h"
#include <esp_log.h>
#include <nvs_flash.h>

#include "enum/SystemState.h"
#include "enum/LockType.h"

#include "AdafruitFingerprintSensor.h"
#include "AdafruitNFCSensor.h"
#include "repository/SDCardModule/SDCardModule.h"
#include "DoorRelay.h"

#include "service/FingerprintService.h"
#include "service/NFCService.h"
#include "service/SyncService.h"

#include "tasks/NFCTask/NFCTask.h"
#include "tasks/FingerprintTask/FingerprintTask.h"

#include "communication/ble/BLEModule.h"
#include "communication/wifi/Wifi.h"
#include "entity/CommandBleData.h"

// Initialize public state
static SystemState systemState = RUNNING;

extern "C" void app_main(void)
{
    // Initialize the NVS Storage for Bluetooth and Wifi credentials
    // https://www.esp32.com/viewtopic.php?t=26365
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
    
    // Initialize the Sensor and Electrical Components
    DoorRelay *doorRelay = new DoorRelay();
    SDCardModule *sdCardModule = new SDCardModule();
    FingerprintSensor *adafruitFingerprintSensor = new AdafruitFingerprintSensor();
    AdafruitNFCSensor *adafruitNFCSensor = new AdafruitNFCSensor();

    // Initializing the Communication Service
    BLEModule *bleModule = new BLEModule();

    // Initialize the Service
    FingerprintService *fingerprintService = new FingerprintService(adafruitFingerprintSensor, sdCardModule, doorRelay, bleModule);
    NFCService *nfcService = new NFCService(adafruitNFCSensor, sdCardModule, doorRelay, bleModule);
    SyncService *syncService = new SyncService(sdCardModule, bleModule);

    // Initialize the Task
    NFCTask *nfcTask = new NFCTask("NFC Task", 1, nfcService);
    FingerprintTask *fingerprintTask = new FingerprintTask("Fingerprint Task", 1, fingerprintService);

    // Setup BLE
    bleModule -> initBLE();
    bleModule -> setupCharacteristic();

    // Start Task
    nfcTask -> startTask();
    fingerprintTask -> startTask();
     
    // Loop Main Mechanism
    while(1){
        ESP_LOGD(LOG_TAG, "Running Main Thread");

        // Access current command data from extern static
        const char* command = commandBleData.getCommand();
        const char* name =  commandBleData.getName();
        const char* key_access =  commandBleData.getKeyAccess();

        switch (systemState) {
            case RUNNING:
                if (command != nullptr) {
                    if (strcmp(command, "register_fp") == 0) {
                        systemState = ENROLL_FP;
                    }
                    if (strcmp(command, "delete_fp") == 0) {
                        systemState = DELETE_FP;
                    }
                    if (strcmp(command, "register_rfid") == 0) {
                        systemState = ENROLL_RFID;
                    }
                    if (strcmp(command, "delete_rfid") == 0) {
                        systemState = DELETE_RFID;
                    }
                    if (strcmp(command, "update_visitor") == 0) {
                        systemState = UPDATE_VISITOR;
                    }
                }
                break;
            
            case ENROLL_RFID:
                ESP_LOGI(LOG_TAG,"Start Registering RFID!");
                nfcTask -> suspendTask();

                nfcService -> addNFC(name);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                
                systemState = RUNNING;
                commandBleData.clear();
                nfcTask -> resumeTask();
                break;
    
            case DELETE_RFID:
                ESP_LOGI(LOG_TAG, "Start Deleting RFID!");
                nfcTask -> suspendTask();

                nfcService -> deleteNFC(name, key_access);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
        
                systemState = RUNNING;
                commandBleData.clear();
                nfcTask -> resumeTask();
                break;
    
            case ENROLL_FP:
                ESP_LOGI(LOG_TAG,"Start Registering Fingerprint!");
                fingerprintTask -> suspendTask();

                fingerprintService -> addFingerprint(name);
                vTaskDelay(1000 / portTICK_PERIOD_MS);

                systemState = RUNNING;
                commandBleData.clear();
                fingerprintTask -> resumeTask();
                break;
    
            case DELETE_FP:
                ESP_LOGI(LOG_TAG, "Start Deleting Fingerprint!");
                fingerprintTask -> suspendTask();

                fingerprintService -> deleteFingerprint(name, atoi(key_access));
                vTaskDelay(1000 / portTICK_PERIOD_MS);
        
                systemState = RUNNING;
                commandBleData.clear();
                fingerprintTask -> resumeTask();
                break;

            case UPDATE_VISITOR:
                ESP_LOGI(LOG_TAG, "Start Sync Data!");
                fingerprintTask -> suspendTask();
                nfcTask -> suspendTask();

                syncService -> sync();
                vTaskDelay(1000 / portTICK_PERIOD_MS);

                systemState = RUNNING;
                commandBleData.clear();
                fingerprintTask -> resumeTask();
                nfcTask -> resumeTask();
                break;
            
        }
        vTaskDelay(50/ portTICK_PERIOD_MS);
    }
}
