#ifndef OTA_H
#define OTA_H

#include <ArduinoOTA.h>

class OTA{
    public:
        OTA();

        void init(void);
        void handleOTA(void);
};

#endif