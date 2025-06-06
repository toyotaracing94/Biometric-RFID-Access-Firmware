#ifndef SDCARD_MODULE_H
#define SDCARD_MODULE_H

#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <Arduino.h>

#include <vector>
#include "enum/LockType.h"

#define CS_PIN 5    // Chip Select pin
#define SCK_PIN 18  // Clock pin
#define MISO_PIN 19 // Master In Slave Out
#define MOSI_PIN 23 // Master Out Slave In

#define FINGERPRINT_FILE_PATH "/fingerprints.json" // File path for storing Fingerprints Access to Data
#define RFID_FILE_PATH "/rfids.json"               // File path for storing NFC Tag to Data

/// @brief SD Card class wrapper
class SDCardModule {
public:
    SDCardModule();
    bool setup();

    bool isFingerprintIdRegistered(int fingerprintId);
    bool saveFingerprintToSDCard(const char *username, int fingerprintId, const char *visitorId, const char *keyAccessId);
    bool deleteFingerprintFromSDCard(const char *keyAccessId);
    bool deleteFingerprintsUserFromSDCard(const char *visitorId);
    int getFingerprintIdByKeyAccessId(const char *keyAccessId);
    std::vector<int> getFingerprintIdsByVisitorId(const char *visitorId);
    std::string* getKeyAccessIdByFingerprintId(int fingerprintId);

    bool isNFCIdRegistered(const char *uidCard);
    bool saveNFCToSDCard(const char *username, const char *uidCard, const char *visitorId, const char *keyAccessId);
    bool deleteNFCFromSDCard(const char *keyAccessId);
    bool deleteNFCsUserFromSDCard(const char *visitorId);
    std::string* getKeyAccessIdByNFCUid(char *uidCard);

    bool deleteAccessJsonFile(LockType type);
    void createEmptyJsonFileIfNotExists(const char *filepath);
    JsonDocument syncData();
};

#endif