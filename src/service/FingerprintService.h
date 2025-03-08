#ifndef FINGERPRINT_SERVICE_H
#define FINGERPRINT_SERVICE_H

#include "managers/FingerprintManager.h"

class FingerprintService {
public:
    FingerprintService(FingerprintManager *fingerprintmanager);
    bool setup();
    bool addFingerprint(char* username, int fingerprintId);
    bool deleteFingerprint(char* username, int fingerprintId);
private:
    FingerprintManager* _fingerprintManager;
};

#endif