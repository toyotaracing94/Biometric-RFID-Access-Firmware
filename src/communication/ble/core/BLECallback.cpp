#include "BLECallback.h"
#include <esp_log.h>
#define BLE_CALLBACK_LOG_TAG "BLE_CALLBACK"


/**
 * @brief Handles the BLE client connection event.
 * 
 * @param pServer Pointer to the NimBLEServer object.
 * @param connInfo Connection information (e.g., MAC address, connection handle).
 */
void BLECallback::onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) {
    std::string clientAddress = connInfo.getAddress().toString();
    ESP_LOGI(BLE_CALLBACK_LOG_TAG, "A device just connected! Address: %s", clientAddress.c_str());

    // Just let open the server advertise
    pServer->startAdvertising();
    ESP_LOGI(BLE_CALLBACK_LOG_TAG, "BLE advertising restarted to remain discoverable");
}

/**
 * @brief Handles the BLE client disconnection event.
 * 
 * @param pServer Pointer to the NimBLEServer object.
 * @param connInfo Connection information at the time of disconnection.
 * @param reason Reason for the disconnection (e.g., timeout, remote closed).
 */
void BLECallback::onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) {
    std::string clientAddress = connInfo.getAddress().toString();
    ESP_LOGW(BLE_CALLBACK_LOG_TAG, "A device just disconnected! Address: %s, Reason: 0x%X", clientAddress.c_str(), reason);

    // Restart advertising so the device can reconnect
    pServer->startAdvertising();
    ESP_LOGI(BLE_CALLBACK_LOG_TAG, "BLE advertising restarted");
}