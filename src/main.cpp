#define LOG_LEVEL_LOCAL ESP_LOG_VERBOSE
#define LOG_TAG "MAIN"

#include "main.h"
#include <esp_log.h>

#include "enum/SystemState.h"
#include "enum/LockType.h"

#include "AdafruitFingerprintSensor.h"
#include "AdafruitNFCSensor.h"
#include "SDCardModule.h"
#include "DoorRelay.h"

#include "service/FingerprintService.h"
#include "service/NFCService.h"

#include "tasks/NFCTask/NFCTask.h"
#include "tasks/FingerprintTask/FingerprintTask.h"

// Initialize public state
static SystemState systemState = RUNNING;
static char* username ;
static char* nfcId;
static int fingerprintId;

extern "C" void app_main(void)
{
    // Initialize the Sensor and Electrical Components
    DoorRelay *doorRelay = new DoorRelay();
    SDCardModule *sdCardModule = new SDCardModule();
    FingerprintSensor *adafruitFingerprintSensor = new AdafruitFingerprintSensor();
    AdafruitNFCSensor *adafruitNFCSensor = new AdafruitNFCSensor();

    // Initialize the Service
    FingerprintService *fingerprintService = new FingerprintService(adafruitFingerprintSensor, sdCardModule, doorRelay);
    NFCService *nfcService = new NFCService(adafruitNFCSensor, sdCardModule, doorRelay);

    // Initialize the Task
    TaskHandle_t nfcTaskHandle;
    NFCTask *nfcTask = new NFCTask("NFC Task", 1, &nfcTaskHandle, nfcService);
    nfcTask -> createTask();
 
    TaskHandle_t fingerprintTaskHandle;
    FingerprintTask *fingerprintTask = new FingerprintTask("Fingerprint Task", 1, &fingerprintTaskHandle, fingerprintService);
    fingerprintTask -> createTask();
     
     // Loop Main Mechanism
    while(1){
        ESP_LOGD(LOG_TAG, "Running Main Thread");
        switch (systemState) {
            case RUNNING:
                // Do nothing I guess
                break;
            
            case ENROLL_RFID:
                ESP_LOGI(LOG_TAG,"Start Registering RFID!");
                nfcService -> addNFC(username);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                
                systemState = RUNNING;
                nfcTask -> resumeTask();
                break;
    
            case DELETE_RFID:
                ESP_LOGI(LOG_TAG, "Start Deleting RFID!");
                nfcService -> deleteNFC(username, nfcId);
        
                systemState = RUNNING;
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                nfcTask -> resumeTask();
                break;
    
            case ENROLL_FP:
                ESP_LOGI(LOG_TAG,"Start Registering Fingerprint!");
                fingerprintService -> addFingerprint(username, fingerprintId);

                systemState = RUNNING;
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                fingerprintTask -> resumeTask();
                break;
    
            case DELETE_FP:
                ESP_LOGI(LOG_TAG, "Start Deleting Fingerprint!");
                fingerprintService -> deleteFingerprint(username, fingerprintId);
        
                systemState = RUNNING;
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                fingerprintTask -> resumeTask();
                break;
        }
        vTaskDelay(50/ portTICK_PERIOD_MS);
    }
}
