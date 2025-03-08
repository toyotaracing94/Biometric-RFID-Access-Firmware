#define LOG_LEVEL_LOCAL ESP_LOG_VERBOSE
#define LOG_TAG "MAIN"

#include "main.h"
#include "esp_log.h"
#include "sensors/FingerprintSensor/AdafruitFingerprintSensor.h"
#include "managers/FingerprintManager.h"
#include "service/FingerprintService.h"

extern "C" void app_main(void)
{
    // AdafruitFingerprintSensor instance
    AdafruitFingerprintSensor adafruitFingerprintSensor;
    FingerprintManager fingerprintManager(&adafruitFingerprintSensor);
    FingerprintService fingerprintService(&fingerprintManager);
}
