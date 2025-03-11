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

    // Sensor and Electrical Setup
    doorRelay -> setup();
    sdCardModule -> setup();

    sdCardModule -> createEmptyJsonFileIfNotExists("/text.json");

}
