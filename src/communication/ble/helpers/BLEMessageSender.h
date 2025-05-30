#ifndef BLE_MESSAGE_SENDER_H
#define BLE_MESSAGE_SENDER_H

#define BLE_MESSAGE_SENDER_LOG_TAG "BLE_MESSAGE_SENDER"

#include "NimBLEDevice.h"
#include <esp_log.h>
#include <ArduinoJson.h>

class BLEMessageSender {
public:
    /**
     * @brief a JSON notification helper class to send status code the connected client based on respected characteristic want's to be used 
     * 
     * This function serializes the JSON document and sends it to the notification characteristic that was being passed.
     * 
     * @param charac The JSON document containing the notification data.
     * @param StatusCode status code of the operation results. See `lib/StatusCode.h`
     */
    static void sendNotification(NimBLECharacteristic* charac, int StatusCode) {
        if (charac) {
            ESP_LOGI(BLE_MESSAGE_SENDER_LOG_TAG, "Sending notification with status code: %d", StatusCode);

            JsonDocument document;
            document["status"] = StatusCode;
            String buffer;
            serializeJson(document, buffer);
            charac -> setValue(buffer.c_str());
            charac -> notify();
            return;
        }
    }
};

#endif