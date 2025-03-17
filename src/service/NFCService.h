#ifndef NFC_SERVICE_H
#define NFC_SERVICE_H

#include "AdafruitNFCSensor.h"
#include "SDCardModule.h"
#include "DoorRelay.h"

/// @brief Class that manages the NFC Access Control system by wrapping the functionalitites of NFC sensor, SD Card module, and the Door Relay
class NFCService {
    public:
        NFCService(AdafruitNFCSensor *nfcSensor, SDCardModule *sdCardModule, DoorRelay *doorRelay);
        bool setup();
        bool addNFC(const char* username);
        bool deleteNFC(const char* username, const char* NFCuid);
        bool authenticateAccessNFC();

    private:
        AdafruitNFCSensor* _nfcSensor;
        SDCardModule* _sdCardModule;
        DoorRelay* _doorRelay;
};

#endif