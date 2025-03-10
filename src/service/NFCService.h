#ifndef NFC_SERVICE_H
#define NFC_SERVICE_H

#include "AdafruitNFCSensor.h"
#include "SDCardModule.h"
#include "DoorRelay.h"

class NFCService {
    public:
        NFCService(AdafruitNFCSensor *nfcSensor, SDCardModule *sdCardModule, DoorRelay *doorRelay);
        bool setup();
        bool addNFC(char* username);
        bool deleteNFC(char* username, char* NFCuid);
        bool authenticateAccessNFC();

    private:
        AdafruitNFCSensor* _nfcSensor;
        SDCardModule* _sdCardModule;
        DoorRelay* _doorRelay;
};

#endif