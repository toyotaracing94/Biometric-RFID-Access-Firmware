#ifndef BLEMODULE_H
#define BLEMODULE_H

#include <BLEServer.h>
#include <BLEDevice.h>
#include <BLEAdvertising.h>

// Define BLE service and characteristic UUIDs
#define BLESERVERNAME               "Yaris Cross Door Auth"

class BLEModule {
public:
    BLEModule();
    void initBLE();
    void setupCharacteristic();

private:
    BLEServer* _bleServer;
    BLEAdvertising* _bleAdvertise;  
};




#endif
