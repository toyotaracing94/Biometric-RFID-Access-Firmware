#ifndef SYNC_SERVICE_H
#define SYNC_SERVICE_H

#include "SDCardModule.h"
#include <esp_log.h>
#include "communication/ble/BLEModule.h"

class SyncService {
    public:
        SyncService(SDCardModule *sdCardModule, BLEModule *bleModule);
        void sync();
    private:
        SDCardModule* _sdCardModule;
        BLEModule* _bleModule;
};

#endif