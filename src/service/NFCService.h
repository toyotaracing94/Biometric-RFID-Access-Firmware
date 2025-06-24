#ifndef NFC_SERVICE_H
#define NFC_SERVICE_H

#include "AdafruitNFCSensor.h"
#include "AdafruitFingerprintSensor.h"
#include "DoorRelay.h"
#include "StatusCodes.h"
#include "repository/SDCardModule/SDCardModule.h"
#include "communication/ble/core/BLEModule.h"
#include "entity/QueueMessage.h"
#include "enum/LockType.h"
#include "enum/SystemState.h"
#include "config/Config.h"

/// @brief Class that manages the NFC Access Control system by wrapping the functionalitites of NFC sensor, SD Card module, and the Door Relay
class NFCService {
    public:
        NFCService(AdafruitNFCSensor *nfcSensor, AdafruitFingerprintSensor *fingerprintSensor, SDCardModule *sdCardModule, DoorRelay *doorRelay, BLEModule *bleModule, QueueHandle_t nfcQueueRequest, QueueHandle_t nfcQueueResponse);
        bool setup();
        bool addNFC(const char *username, const char *visitorId, const char *keyAccessId);
        bool deleteNFC(const char *keyAccessId);
        bool deleteNFCsUser(const char *visitorId);
        bool deleteNFCAccessFile();
        bool authenticateAccessNFC();

        // Helper functions
        void sendbleNotification(int statusCode);
        void sendbleNotification(const char* status, const char* username, const char* visitorId, const char* message, const char *type);
        bool handleError(int statusCode, const char* username, const char* visitorId, const char* message, bool cleanup);
        bool handleDeleteError(int statusCode, const char* message);
        void addNFCCallback(int statusCode);

    private:
        AdafruitNFCSensor* _nfcSensor;
        AdafruitFingerprintSensor* _fingerprintSensor;
        SDCardModule* _sdCardModule;
        DoorRelay* _doorRelay;
        BLEModule* _bleModule;
        QueueHandle_t _nfcQueueRequest;
        QueueHandle_t _nfcQueueResponse;
};

#endif