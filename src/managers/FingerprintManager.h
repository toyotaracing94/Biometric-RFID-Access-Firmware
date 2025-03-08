#ifndef FINGERPRINT_MANAGER_H
#define FINGERPRINT_MANAGER_H

#include "FingerprintSensor.h"

class FingerprintManager {
public:
    FingerprintManager(FingerprintSensor *sensor);
    bool setup();
    bool addFingerprintModel(int id);
    bool deleteFingerprintModel(int id);

private:
    FingerprintSensor* _sensor;
};

#endif
