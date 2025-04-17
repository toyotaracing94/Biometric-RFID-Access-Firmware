#ifndef NFC_SERVICE_H
#define NFC_SERVICE_H

#include "AdafruitNFCSensor.h"
#include "DoorRelay.h"
#include "StatusCodes.h"
#include "repository/SDCardModule/SDCardModule.h"
#include "communication/ble/BLEModule.h"
#include "entity/QueueMessage.h"
#include "enum/LockType.h"
#include "enum/OperationState.h"
#include "config/Config.h"

/// @brief Class that manages the NFC Access Control system by wrapping the functionalitites of NFC sensor, SD Card module, and the Door Relay
class NFCService {
    public:
        NFCService(AdafruitNFCSensor *nfcSensor, SDCardModule *sdCardModule, DoorRelay *doorRelay, BLEModule *bleModule, QueueHandle_t nfcQueueRequest, QueueHandle_t nfcQueueResponse);
        bool setup();
        bool addNFC(const char* username);
        bool deleteNFC(const char *visitorId);
        bool authenticateAccessNFC();

        // Helper functions
        void sendbleNotification(const char* status, const char* username, const char* visitorId, const char* message, const char *type);
        bool handleError(const char* username, const char* visitorId, const char* message, bool cleanup);
        bool handleDeleteError(const char* visitorId, const char* message);

    private:
        AdafruitNFCSensor* _nfcSensor;
        SDCardModule* _sdCardModule;
        DoorRelay* _doorRelay;
        BLEModule* _bleModule;
        QueueHandle_t _nfcQueueRequest;
        QueueHandle_t _nfcQueueResponse;
};

#endif