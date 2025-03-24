#ifndef BLE_CALLBACK
#define BLE_CALLBACK

#include <BLEServer.h>
#include <BLEDevice.h>

/// @brief The BLECallback class handles BLE server connection events trigger
class BLECallback : public BLEServerCallbacks {  
    public:
        void onConnect(BLEServer *pServer) override;
        void onDisconnect(BLEServer *pServer) override;
};  

#endif