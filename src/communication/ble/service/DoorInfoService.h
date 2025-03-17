#ifndef DOOR_INFO_SERVICE_H
#define DOOR_INFO_SERVICE_H

#define DOOR_INFO_SERVICE_LOG_TAG "DOOR_INFO_SERVICE"

#include <BLEServer.h>
#include <BLEService.h>
#include <BLECharacteristic.h>
#include <BLE2902.h>
#include <esp_log.h>
#include <ArduinoJson.h>

#define SERVICE_UUID                         "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define DOOR_CHARACTERISTIC_UUID             "ce51316c-d0e1-4ddf-b453-bda6477ee9b9"
#define LED_REMOTE_CHARACTERISTIC_UUID       "beb5483e-36e1-4688-b7f5-ea07361b26a8"  
#define AC_REMOTE_CHARACTERISTIC_UUID        "9f54dbb9-e127-4d80-a1ab-413d48023ad2"  
#define NOTIFICATION_CHARACTERISTIC_UUID     "01952383-cf1a-705c-8744-2eee6f3f80c8"

class DoorCharacteristicCallbacks : public BLECharacteristicCallbacks {
    public:
        void onWrite(BLECharacteristic* pCharacteristic) override;
};

class ACCharacteristicCallback : public BLECharacteristicCallbacks {
    public:
        void onWrite(BLECharacteristic* pCharacteristic) override;
};

class LEDRemoteCharacteristicCallback : public BLECharacteristicCallbacks {
    public:
        void onWrite(BLECharacteristic* pCharacteristic) override;
};


class DoorInfoService {
    public:
        DoorInfoService(BLEServer* pServer);
        ~DoorInfoService();
        void startService();
        void sendNotification(JsonDocument& json);

    private:
        BLEServer* _pServer;
        BLEService* _pService;
        BLECharacteristic* _pDoorChar;
        BLECharacteristic* _pACChar;
        BLECharacteristic* _pLedChar;
        BLECharacteristic* _pNotificationChar;
};

#endif