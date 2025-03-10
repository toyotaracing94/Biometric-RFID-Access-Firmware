#define LOG_LEVEL_LOCAL ESP_LOG_VERBOSE
#define LOG_TAG "MAIN"

#include "main.h"
#include <esp_log.h>

#include "AdafruitFingerprintSensor.h"
#include "AdafruitNFCSensor.h"
#include "SDCardModule.h"
#include "DoorRelay.h"

#include "service/FingerprintService.h"

extern "C" void app_main(void)
{
    // Initialize the Sensor and Electrical Components
    DoorRelay *doorRelay = new DoorRelay();
    SDCardModule *sdCardModule = new SDCardModule();
    FingerprintSensor *adafruitFingerprintSensor = new AdafruitFingerprintSensor();
    AdafruitNFCSensor *adafruitNFCSensor = new AdafruitNFCSensor();

    // Initialize the Service
    FingerprintService *fingerprintService = new FingerprintService(adafruitFingerprintSensor, sdCardModule, doorRelay);

    doorRelay -> toggleRelay();
    
    // Try first
    // char username[10] = "Jun";
    // fingerprintService -> addFingerprint(username, 3);
}
