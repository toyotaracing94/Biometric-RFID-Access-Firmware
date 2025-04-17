#include "BLECallback.h"
#include <esp_log.h>
#define BLE_CALLBACK_LOG_TAG "BLE_CALLBACK"


/**
 * @brief	Handles the BLE client connection event.
 * 
 * @param pServer Pointer to the BLEServer object.
 * 
 * @return None
 */
void BLECallback::onConnect(BLEServer *pServer){
    ESP_LOGI(BLE_CALLBACK_LOG_TAG, "A Device just connected! Device %s", BLEDevice::toString().c_str());
}

/**
 * @brief Handles when the BLE client disconnected event.
 * 
 * @param pServer Pointer to the BLEServer object.
 * 
 * @return None
 */
void BLECallback::onDisconnect(BLEServer *pServer){
    ESP_LOGI(BLE_CALLBACK_LOG_TAG, "A Device just disconnected! Device %s", BLEDevice::toString().c_str());
    pServer -> startAdvertising();
}
