#include "DeviceInfoService.h"

DeviceInfoService::DeviceInfoService(NimBLEServer* pServer) {
    this -> _pServer = pServer;
    this -> _pService = nullptr;
    this -> _pManufacturerNameChar = nullptr;
    this -> _pFirmwareRevisionChar = nullptr;
}

DeviceInfoService::~DeviceInfoService() {}

/**
 * @brief Start the DeviceInfoService
 * 
 * 
 */
void DeviceInfoService::startService() {
    ESP_LOGI(DEVICE_INFO_SERVICE_LOG_TAG, "Initializing BLE Device Info Service");
    _pService = _pServer -> createService(DEVICE_INFORMATION_SERVICE_UUID);

    _pManufacturerNameChar = _pService -> createCharacteristic(
        MANUFACTURER_NAME_UUID,
        NIMBLE_PROPERTY::READ
    );
    _pManufacturerNameChar -> setValue("Capability Center Division");

    _pFirmwareRevisionChar = _pService->createCharacteristic(
        FIRMWARE_REVISION_UUID,
        NIMBLE_PROPERTY::READ
    );
    _pFirmwareRevisionChar -> setValue(CURRENT_FIRMWARE_VERSION);
    _pService -> start();
}
