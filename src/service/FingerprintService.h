#ifndef FINGERPRINT_SERVICE_H
#define FINGERPRINT_SERVICE_H

#include "FingerprintSensor.h"
#include "SDCardModule.h"
#include "DoorRelay.h"

class FingerprintService {
public:
    FingerprintService(FingerprintSensor *fingerprintSensor, SDCardModule *sdCardModule, DoorRelay *DoorRelay);
    bool setup();
    bool addFingerprint(char* username, int fingerprintId);
    bool deleteFingerprint(char* username, int fingerprintId);
    bool authenticateAccessFingerprint();

private:
    FingerprintSensor* _fingerprintSensor;
    SDCardModule* _sdCardModule;
    DoorRelay* _doorRelay;
};

#endif