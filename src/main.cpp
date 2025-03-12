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
        ESP_LOGI(LOG_TAG, "Running Main Task");
        vTaskDelay(1000/ portTICK_PERIOD_MS);
    }

}
