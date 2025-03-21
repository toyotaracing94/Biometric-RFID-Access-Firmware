#include "DeviceInfoService.h"

DeviceInfoService::DeviceInfoService(BLEServer* pServer) {
    this -> _pServer = pServer;
    this -> _pService = nullptr;
    this -> _pManufacturerNameChar = nullptr;
    this -> _pFirmwareRevisionChar = nullptr;
}

DeviceInfoService::~DeviceInfoService() {}

void DeviceInfoService::startService() {
    ESP_LOGI(DEVICE_INFO_SERVICE_LOG_TAG, "Initializing BLE Device Info Service");
    _pService = _pServer -> createService(DEVICE_INFORMATION_SERVICE_UUID);

    _pManufacturerNameChar = _pService -> createCharacteristic(
        MANUFACTURER_NAME_UUID,
        BLECharacteristic::PROPERTY_READ
    );
    _pManufacturerNameChar -> setValue("Capability Center Division");

    _pFirmwareRevisionChar = _pService->createCharacteristic(
        FIRMWARE_REVISION_UUID,
        BLECharacteristic::PROPERTY_READ
    );
    _pFirmwareRevisionChar -> setValue(CURRENT_FIRMWARE_VERSION);
    _pService -> start();
}
