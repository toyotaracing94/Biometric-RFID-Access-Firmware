#ifndef DOOR_RELAY_H
#define DOOR_RELAY_H

#define RELAY_UNLOCK_PIN GPIO_NUM_25 /*!< GPIO25 Pin for Relay Unlock  */
#define RELAY_LOCK_PIN GPIO_NUM_26   /*!< GPIO26 Pin for Relay Lock    */

#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

///! Class to wrap the Relay operation of the Lock/Unlock Door State
class DoorRelay
{
public:
    int stateLockCounter = 0;
    DoorRelay();
    bool setup();
    void toggleRelay();

private:
    bool toggleState = false;
};

#endif