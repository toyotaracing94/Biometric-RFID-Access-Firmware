#ifndef BLEMODULE_H
#define BLEMODULE_H

#include <BLEServer.h>
#include <BLEService.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEAdvertising.h>
#include <BLECharacteristic.h>

// Define BLE service and characteristic UUIDs
#define BLESERVERNAME               "Yaris Cross Door Auth"
#define SERVICE_UUID                "4fafc201-1fb5-459e-8fcc-c5c9c331914b"

class BLEModule {
public:
    BLEModule();
    void initBLE();
    void setupCharacteristic();

private:
    BLEServer* _bleServer;
    BLEService* _bleService;
    BLEAdvertising* _bleAdvertise;

    
};




#endif
