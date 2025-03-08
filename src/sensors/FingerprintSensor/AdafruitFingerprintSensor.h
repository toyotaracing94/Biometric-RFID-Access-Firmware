// AdafruitFingerprintSensor.h

#ifndef ADAFRUIT_FINGERPRINT_SENSOR_H
#define ADAFRUIT_FINGERPRINT_SENSOR_H

#include "FingerprintSensor.h"
#include <Adafruit_Fingerprint.h>

#define RX_PIN  16                 /*!< Pin RX to TX Fingerprint */
#define TX_PIN  17                 /*!< Pin TX to RX Fingerprint */
#define UART_NR 2                   
#define BAUD_RATE_FINGERPRINT 57600

class AdafruitFingerprintSensor : public FingerprintSensor {
  public:
    AdafruitFingerprintSensor();
    bool setup() override;
    bool addFingerprintModel(int id) override;
    bool deleteFingerprintModel(int id) override;
  
  private:
    HardwareSerial _serial;
    Adafruit_Fingerprint _fingerprintSensor;
};

#endif
