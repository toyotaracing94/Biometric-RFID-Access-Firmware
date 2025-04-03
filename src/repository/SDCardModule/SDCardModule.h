#ifndef SDCARD_MODULE_H
#define SDCARD_MODULE_H

#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <unordered_set>

#define CS_PIN 5    // Chip Select pin
#define SCK_PIN 18  // Clock pin
#define MISO_PIN 19 // Master In Slave Out
#define MOSI_PIN 23 // Master Out Slave In

#define FINGERPRINT_FILE_PATH "/fingerprints.json" // File path for storing Fingerprints Access to Data
#define RFID_FILE_PATH "/rfids.json"               // File path for storing NFC Tag to Data

/// @brief SD Card class wrapper
class SDCardModule
{
public:
    SDCardModule();
    bool setup();
    bool isFingerprintIdRegistered(int id);
    bool saveFingerprintToSDCard(const char *username, int id, const char *visitorId);
    bool deleteFingerprintFromSDCard(const char *visitorId);
    bool getFingerprintModelByVisitorId(const char *visitorId, JsonObject &result);
    JsonObject getFingerprintModelById(int id);
    bool isNFCIdRegistered(char *id);
    bool saveNFCToSDCard(const char *username, char *id, const char *visitorId);
    bool deleteNFCFromSDCard(const char *username, const char *id);
    void createEmptyJsonFileIfNotExists(const char *filepath);

    bool loadAllFingerprintIds(std::unordered_set<int> &idSet);
    JsonDocument syncData();
};

#endif