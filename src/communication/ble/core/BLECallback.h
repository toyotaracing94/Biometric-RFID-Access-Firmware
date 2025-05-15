#ifndef BLE_CALLBACK
#define BLE_CALLBACK

#include "NimBLEDevice.h"

/// @brief The BLECallback class handles BLE server connection events trigger
class BLECallback : public NimBLEServerCallbacks{  
    public:
        void onConnect(NimBLEServer*pServer, NimBLEConnInfo& connInfo) override;
        void onDisconnect(NimBLEServer*pServer, NimBLEConnInfo& connInfo, int reason) override;
};  

#endif