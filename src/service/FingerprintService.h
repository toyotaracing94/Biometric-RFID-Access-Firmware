#ifndef FINGERPRINT_SERVICE_H
#define FINGERPRINT_SERVICE_H

#include "FingerprintSensor.h"
#include "DoorRelay.h"
#include "repository/SDCardModule/SDCardModule.h"
#include "communication/ble/BLEModule.h"

/// @brief Class that manages the Fingerprint Access Control system by wrapping the functionalitites of Fingerprint sensor, SD Card module, and the Door Relay
class FingerprintService {
public:
    FingerprintService(FingerprintSensor *fingerprintSensor, SDCardModule *sdCardModule, DoorRelay *DoorRelay, BLEModule* bleModule, QueueHandle_t fingerprintToWiFiQueue);
    bool setup();
    bool addFingerprint(const char* username);
    bool deleteFingerprint(const char* username, int fingerprintId);
    bool authenticateAccessFingerprint();
    uint8_t generateFingerprintId();
    void sendbleNotification(char* status, const char* username, int fingerprintId, char* message);

private:
    FingerprintSensor* _fingerprintSensor;
    SDCardModule* _sdCardModule;
    DoorRelay* _doorRelay;
    BLEModule* _bleModule;
    QueueHandle_t _fingerprintToWiFiQueue;
};

#endif