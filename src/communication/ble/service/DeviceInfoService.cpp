#include "DeviceInfoService.h"

DeviceInfoService::DeviceInfoService(BLEServer* pServer) {
    this->pServer = pServer;
    this->pService = nullptr;
    this->pManufacturerNameChar = nullptr;
    this->pFirmwareRevisionChar = nullptr;
}

DeviceInfoService::~DeviceInfoService() { }

void DeviceInfoService::startService() {
    ESP_LOGI(DEVICE_INFO_SERVICE_LOG_TAG, "Initializing BLE Device Info Service");
    pService = pServer -> createService(DEVICE_INFORMATION_SERVICE_UUID);

    pManufacturerNameChar = pService -> createCharacteristic(
        MANUFACTURER_NAME_UUID,
        BLECharacteristic::PROPERTY_READ
    );
    pManufacturerNameChar -> setValue("Capability Center Division");

    pFirmwareRevisionChar = pService->createCharacteristic(
        FIRMWARE_REVISION_UUID,
        BLECharacteristic::PROPERTY_READ
    );
    pFirmwareRevisionChar -> setValue("0.2.0");
    pService->start();
}
