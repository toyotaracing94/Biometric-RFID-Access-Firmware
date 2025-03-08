#ifndef FINGERPRINT_SERVICE_H
#define FINGERPRINT_SERVICE_H

#include "managers/FingerprintManager.h"
#include "SDCardModule.h"

class FingerprintService {
public:
    FingerprintService(FingerprintManager *fingerprintmanager, SDCardModule *sdCardModule);
    bool setup();
    bool addFingerprint(char* username, int fingerprintId);
    bool deleteFingerprint(char* username, int fingerprintId);
private:
    FingerprintManager* _fingerprintManager;
    SDCardModule* _sdCardModule;
};

#endif