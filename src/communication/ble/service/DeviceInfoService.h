#ifndef DEVICEINFOSERVICE_H
#define DEVICEINFOSERVICE_H
#define DEVICE_INFO_SERVICE_LOG_TAG "DEVICE INFO SERVICE"

#include "NimBLEDevice.h"
#include <esp_log.h>
#include "versionInfo.h"

// UUIDs for Device Information Service and its Characteristics
#define DEVICE_INFORMATION_SERVICE_UUID        "180A"
#define MANUFACTURER_NAME_UUID                 "2A29"
#define FIRMWARE_REVISION_UUID                 "2A26"

class DeviceInfoService {
public:
    DeviceInfoService(NimBLEServer* pServer);
    ~DeviceInfoService();
    void startService();

private:
    NimBLEServer* _pServer;
    NimBLEService* _pService;
    NimBLECharacteristic* _pManufacturerNameChar;
    NimBLECharacteristic* _pFirmwareRevisionChar;
};

#endif
