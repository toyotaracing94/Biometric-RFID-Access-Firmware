#ifndef WIFI_H
#define WIFI_H

#include "WiFiManager.h"
#include "enum/WifiState.h"

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

/// @brief Class wrapper for WiFo operation. For now it include api service in here
class Wifi{
    public:
        Wifi(const char* ssid = "ESP32-AP", QueueHandle_t statusQueue = NULL);

        void init(void);
        bool isConnected(void);
        bool reconnect(void);
        void resetSettings(void);
        const char* getSSID(void);
        const char* getIpAddress(void);
        void updateStatus(bool status);
        wl_status_t get_state(void);
        std::string sendPostRequest(const std::string &url, const std::string &payload);
        std::string sendGetRequest(const std::string &url);
        std::string sendDeleteRequest(const std::string &url);

    private:
        const char* _apName;
        QueueHandle_t _wifiStatusQueue;
        WiFiManager _wifiManager;
};

#endif