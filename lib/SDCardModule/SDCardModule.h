#ifndef SDCARD_MODULE_H
#define SDCARD_MODULE_H

#include <SPI.h>
#include <SD.h>

#define CS_PIN 5        // Chip Select pin
#define SCK_PIN 18      // Clock pin
#define MISO_PIN 19     // Master In Slave Out
#define MOSI_PIN 23     // Master Out Slave In

#define FINGERPRINT_FILE_PATH "/fingerprints.json"
#define RFID_FILE_PATH "/rfids.json"

const size_t STACK_JSON_CAPACITY = 256;
const size_t DYNAMIC_JSON_CAPACITY = 1024;

class SDCardModule {
    public:
        SDCardModule();
        bool setup();
        bool isFingerprintIdRegistered(int id);
        bool saveFingerprintToSDCard(char* username, int id);
        bool deleteFingerprintFromSDCard(char* username, int id);

        bool isNFCIdRegistered(char* id);
        bool saveNFCToSDCard(char* username, char* id);
        bool deleteNFCFromSDCard(char* username, char* id);
        void createEmptyJsonFileIfNotExists(const char* filepath);
};

#endif