#ifndef FINGERPRINT_SERVICE_H
#define FINGERPRINT_SERVICE_H

#include "FingerprintSensor.h"
#include "DoorRelay.h"
#include "repository/SDCardModule/SDCardModule.h"
#include "communication/ble/BLEModule.h"
#include "communication/wifi/service/WiFiModule.h"

/// @brief Class that manages the Fingerprint Access Control system by wrapping the functionalitites of Fingerprint sensor, SD Card module, and the Door Relay
class FingerprintService
{
public:
    FingerprintService(FingerprintSensor *fingerprintSensor, SDCardModule *sdCardModule, DoorRelay *DoorRelay, BLEModule *bleModule, WiFiModule *wifiModule);
    bool setup();
    bool addFingerprint(const char *username);
    bool deleteFingerprint(const char *visitorId);
    bool authenticateAccessFingerprint();
    uint8_t generateFingerprintId();
    void sendbleNotification(const char *status, const char *username, const char *visitorId, const char *type, const char *message);

private:
    FingerprintSensor *_fingerprintSensor;
    SDCardModule *_sdCardModule;
    DoorRelay *_doorRelay;
    BLEModule *_bleModule;
    WiFiModule *_wifiModule;
};

#endif