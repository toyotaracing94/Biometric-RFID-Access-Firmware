#define LOG_LEVEL_LOCAL ESP_LOG_VERBOSE
#define LOG_TAG "MAIN"

#include "main.h"
#include "esp_log.h"

#include "AdafruitFingerprintSensor.h"
#include "SDCardModule.h"

#include "service/FingerprintService.h"

extern "C" void app_main(void)
{
    // Initialize the Sensor and Electrical Components
    FingerprintSensor *adafruitFingerprintSensor = new AdafruitFingerprintSensor();
    SDCardModule *sdCardModule = new SDCardModule();

    // Initialize the Service
    FingerprintService fingerprintService(adafruitFingerprintSensor, sdCardModule);

    // Try first
    char username[10] = "Jun";
    fingerprintService.addFingerprint(username, 3);
}
