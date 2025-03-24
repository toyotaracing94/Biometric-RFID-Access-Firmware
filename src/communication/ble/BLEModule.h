#ifndef BLEMODULE_H
#define BLEMODULE_H

#include <BLEServer.h>
#include <BLEDevice.h>
#include <BLEAdvertising.h>

#include "BLECallback.h"

#include "communication/ble/service/DeviceInfoService.h"
#include "communication/ble/service/DoorInfoService.h"

// Define BLE service and characteristic UUIDs
#define BLESERVERNAME               "Yaris Cross Door Auth"

class BLEModule {
public:
    BLEModule();
    void initBLE();
    void setupCharacteristic();
    void sendReport(const char* status, const JsonObject& payload, const char* message);

private:
    BLEServer* _bleServer;
    BLEAdvertising* _bleAdvertise;

    DeviceInfoService* _deviceInfoService;
    DoorInfoService* _doorInfoService;
};




#endif
