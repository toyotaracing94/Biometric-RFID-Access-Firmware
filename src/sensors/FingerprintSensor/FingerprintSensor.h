#ifndef FINGERPRINT_SENSOR_H
#define FINGERPRINT_SENSOR_H

class FingerprintSensor {
public:
    virtual bool setup() = 0;
    virtual bool addFingerprintModel(int id) = 0;
    virtual bool deleteFingerprintModel(int id) = 0;
};

#endif
