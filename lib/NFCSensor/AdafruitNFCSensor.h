#ifndef ADAFRUIT_RFID_SENSOR_H
#define ADAFRUIT_RFID_SENSOR_H

#include <Adafruit_PN532.h>

#define SDA_PIN 21            // Pin SDA to PN532
#define SCL_PIN 22            // Pin SCL to PN532


class AdafruitNFCSensor {
    public:
        AdafruitNFCSensor();
        bool setup();
    
    private:
        Adafruit_PN532 _pn532Sensor;
  };


#endif