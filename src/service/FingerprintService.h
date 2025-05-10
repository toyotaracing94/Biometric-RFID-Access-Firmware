#ifndef FINGERPRINT_SERVICE_H
#define FINGERPRINT_SERVICE_H

#include "FingerprintSensor.h"
#include "DoorRelay.h"
#include "repository/SDCardModule/SDCardModule.h"
#include "config/Config.h"
#include "enum/LockType.h"
#include "entity/QueueMessage.h"
#include "communication/ble/BLEModule.h"

/// @brief Class that manages the Fingerprint Access Control system by wrapping the functionalitites of Fingerprint sensor, SD Card module, and the Door Relay
class FingerprintService
{
public:
    FingerprintService(FingerprintSensor *fingerprintSensor, SDCardModule *sdCardModule, DoorRelay *DoorRelay, BLEModule* bleModule, QueueHandle_t fingerprintQueueRequest, QueueHandle_t fingerprintQueueResponse);
    bool setup();
    bool addFingerprint(const char *username, const char *visitorId, const char *keyAccessId);
    bool deleteFingerprint(const char *keyAccessId);
    bool deleteFingerprintsUser(const char *visitorId);
    bool deleteAllFingerprintModel();
    bool authenticateAccessFingerprint();
    uint8_t generateFingerprintId();

    // Helper functions
    void sendbleNotification(int statusCode);
    void sendbleNotification(const char *status, const char *username, const char *keyAccessId, const char *type, const char *message);
    bool handleError(int statusCode, const char* username, const char* keyAccessId, const char* message, bool cleanup);
    bool handleDeleteError(int statusCode, const char* message);
    void addFingerprintCallback(int statusCode);

private:
    FingerprintSensor* _fingerprintSensor;
    SDCardModule* _sdCardModule;
    DoorRelay* _doorRelay;
    BLEModule* _bleModule;
    QueueHandle_t _fingerprintQueueRequest;
    QueueHandle_t _fingerprintQueueResponse;
};

#endif