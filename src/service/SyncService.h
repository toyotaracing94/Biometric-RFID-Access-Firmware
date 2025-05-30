#ifndef SYNC_SERVICE_H
#define SYNC_SERVICE_H

#include "repository/SDCardModule/SDCardModule.h"
#include "communication/ble/core/BLEModule.h"
#include <esp_log.h>

class SyncService {
    public:
        SyncService(SDCardModule *sdCardModule, BLEModule *bleModule);
        void sync();
    private:
        SDCardModule* _sdCardModule;
        BLEModule* _bleModule;
};

#endif