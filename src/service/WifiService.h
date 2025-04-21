#ifndef WIFI_SERVICE_H
#define WIFI_SERVICE_H

#include "communication/wifi/Wifi.h"
#include "communication/ble/BLEModule.h"

#include "ota/ota.h"

#include "entity/QueueMessage.h"
#include "enum/LockType.h"

#include "repository/SDCardModule/SDCardModule.h"

/// @brief Class that manages WiFi Service to send api requests
class WifiService {
    public:
        WifiService(BLEModule *bleModule, OTA *otaModule, SDCardModule *sdCardModule);
        bool setup();
        bool isConnected();
        bool reconnect();

        NFCQueueResponse addNFCToServer(NFCQueueRequest nfcRequest);
        NFCQueueResponse deleteNFCFromServer(NFCQueueRequest nfcRequest);
        NFCQueueResponse authenticateAccessNFCToServer(NFCQueueRequest nfcRequest);

        FingerprintQueueResponse addFingerprintToServer(FingerprintQueueRequest fingerprintRequest);
        FingerprintQueueResponse deleteFingerprintFromServer(FingerprintQueueRequest fingerprintRequest);
        FingerprintQueueResponse authenticateAccessFingerprintToServer(FingerprintQueueRequest fingerprintRequest);

        void beginOTA();
        void handleOTA();

    private:
        BLEModule* _bleModule;
        OTA* _otaModule;
        SDCardModule* _sdCardModule;
        Wifi* _wifi;
};

#endif