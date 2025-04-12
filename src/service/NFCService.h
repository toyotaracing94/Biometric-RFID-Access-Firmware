#ifndef NFC_SERVICE_H
#define NFC_SERVICE_H

#include "AdafruitNFCSensor.h"
#include "DoorRelay.h"
#include "repository/SDCardModule/SDCardModule.h"
#include "communication/ble/BLEModule.h"
#include "entity/QueueMessage.h"

/// @brief Class that manages the NFC Access Control system by wrapping the functionalitites of NFC sensor, SD Card module, and the Door Relay
class NFCService {
    public:
        NFCService(AdafruitNFCSensor *nfcSensor, SDCardModule *sdCardModule, DoorRelay *doorRelay, BLEModule *bleModule, QueueHandle_t nfcToWiFiQueue);
        bool setup();
        bool addNFC(const char* username);
        bool deleteNFC(const char* username, const char* NFCuid);
        bool authenticateAccessNFC();
        void sendbleNotification(char* status, const char* username, const char* uidCard, char* message);

    private:
        AdafruitNFCSensor* _nfcSensor;
        SDCardModule* _sdCardModule;
        DoorRelay* _doorRelay;
        BLEModule* _bleModule;
        QueueHandle_t _nfcToWiFiQueue;
};

#endif