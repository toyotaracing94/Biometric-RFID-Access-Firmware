#ifndef FINGERPRINT_SENSOR_H
#define FINGERPRINT_SENSOR_H

#include "ErrorCodes.h"

/// @brief Base class for any Fingerprint sensor operation
class FingerprintSensor
{
public:
    virtual bool setup() = 0;
    virtual int getFingerprintIdModel() = 0;
    virtual bool addFingerprintModel(int id) = 0;
    virtual bool verifyFingerprint(int id) = 0;
    virtual bool deleteFingerprintModel(int id) = 0;
};

#endif
