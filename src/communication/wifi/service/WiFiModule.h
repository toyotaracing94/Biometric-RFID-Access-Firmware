#ifndef WIFIMODULE_H
#define WIFIMODULE_H
#include <WiFi.h>
#include <HTTPClient.h>
#include <string>
#include <WiFiClient.h>
class WiFiModule
{
public:
    WiFiModule();
    bool connectWifi();
    bool isConnected();
    std::string sendPostRequest(const std::string &url, const std::string &payload);
    std::string sendGetRequest(const std::string &url);
    std::string sendDeleteRequest(const std::string &url);
};
#endif // WIFIMODULE_H
