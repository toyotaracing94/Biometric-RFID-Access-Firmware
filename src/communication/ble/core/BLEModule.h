#ifndef BLEMODULE_H
#define BLEMODULE_H

#include "NimBLEDevice.h"

#include "BLECallback.h"

#include "communication/ble/service/DeviceInfoService.h"
#include "communication/ble/service/DoorInfoService.h"

// Define BLE service and characteristic UUIDs
#define BLESERVERNAME               "Yaris Door Auth"

class BLEModule {
public:
    BLEModule();
    void initBLE();
    void setupCharacteristic();
    void setupAdvertising();
    void sendReport(const char* status, const JsonObject& payload, const char* message);
    void sendReport(int statusCode);

private:
    NimBLEServer* _bleServer;
    NimBLEAdvertising* _bleAdvertise;

    DeviceInfoService* _deviceInfoService;
    DoorInfoService* _doorInfoService;
};




#endif
