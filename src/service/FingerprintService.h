#ifndef FINGERPRINT_SERVICE_H
#define FINGERPRINT_SERVICE_H

#include "FingerprintSensor.h"
#include "SDCardModule.h"

class FingerprintService {
public:
    FingerprintService(FingerprintSensor *fingerprintSensor, SDCardModule *sdCardModule);
    bool setup();
    bool addFingerprint(char* username, int fingerprintId);
    bool deleteFingerprint(char* username, int fingerprintId);
private:
    FingerprintSensor* _fingerprintSensor;
    SDCardModule* _sdCardModule;
};

#endif