#ifndef NFC_SERVICE_H
#define NFC_SERVICE_H

#include "AdafruitNFCSensor.h"
#include "DoorRelay.h"
#include "repository/SDCardModule/SDCardModule.h"
#include "communication/ble/BLEModule.h"
#include "communication/wifi/service/WiFiModule.h"

/// @brief Class that manages the NFC Access Control system by wrapping the functionalitites of NFC sensor, SD Card module, and the Door Relay
class NFCService
{
public:
    NFCService(AdafruitNFCSensor *nfcSensor, SDCardModule *sdCardModule, DoorRelay *doorRelay, BLEModule *bleModule, WiFiModule *wifiModule);
    bool setup();
    bool addNFC(const char *username);
    bool deleteNFC(const char *visitorId);
    bool authenticateAccessNFC();
    void sendbleNotification(const char *status, const char *username, const char *uidCard, const char *message, const char *type);

private:
    AdafruitNFCSensor *_nfcSensor;
    SDCardModule *_sdCardModule;
    DoorRelay *_doorRelay;
    BLEModule *_bleModule;
    WiFiModule *_wifiModule;
};

#endif