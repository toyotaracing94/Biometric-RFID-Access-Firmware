#ifndef DOOR_RELAY_H
#define DOOR_RELAY_H

#define RELAY_UNLOCK_PIN GPIO_NUM_25
#define RELAY_LOCK_PIN GPIO_NUM_26

#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class DoorRelay {
    public:
        int stateLockCounter = 0;
        DoorRelay();
        bool setup();
        void toogleRelay();

    private:
        bool toogleState = false;
};

#endif