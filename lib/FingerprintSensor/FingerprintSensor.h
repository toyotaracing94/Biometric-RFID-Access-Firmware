#ifndef FINGERPRINT_SENSOR_H
#define FINGERPRINT_SENSOR_H

#include "FingerprintErrorCode.h"

class FingerprintSensor {
public:
    virtual bool setup() = 0;
    virtual int getFingerprintIdModel() = 0;
    virtual bool addFingerprintModel(int id) = 0;
    virtual bool deleteFingerprintModel(int id) = 0;

    virtual void toggleSuccessFingerprintLED(void);
    virtual void toggleFailedFingerprintLED(void);
};

#endif
