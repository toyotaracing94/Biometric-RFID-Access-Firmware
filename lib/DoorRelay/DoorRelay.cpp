#define DOOR_RELAY_LOG_TAG "DOOR_RELAY"
#include <esp_log.h>
#include "DoorRelay.h"

DoorRelay::DoorRelay(){
    setup();
}

/**
 * @brief   DoorRelay setup
 *
 * Initializes the GPIO pins used to control the door relay (unlock and lock) by setting their direction
 * to output and setting their initial level to high (1).
 * If any configuration step fails, the function returns false, indicating the setup process was unsuccessful.
 *
 * @return
 *     - true  : GPIO pins for the relay have been successfully configured
 *     - false : An error occurred while setting the GPIO pins direction or level
 */
bool DoorRelay::setup(){
    if (gpio_set_direction(RELAY_UNLOCK_PIN, GPIO_MODE_OUTPUT) != ESP_OK){
        return false;
    }
    if (gpio_set_direction(RELAY_LOCK_PIN, GPIO_MODE_OUTPUT) != ESP_OK){
        return false;
    }
    if (gpio_set_level(RELAY_UNLOCK_PIN, 1) != ESP_OK){
        return false;
    }
    if (gpio_set_level(RELAY_LOCK_PIN, 1) != ESP_OK){
        return false;
    }
    return true;
}

/**
 * @brief	Toggles the door relay between unlock and lock state
 *
 * Activates the unlock or lock relay based on `toggleState`.
 * The `toggleState` is flipped after each call
 *
 * @return None
 */
void DoorRelay::toggleRelay(){
    if (toggleState){
        ESP_LOGI(DOOR_RELAY_LOG_TAG, "relayUnlock ON (DOOR UNLOCK)...");
        gpio_set_level(RELAY_UNLOCK_PIN, 0);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        gpio_set_level(RELAY_UNLOCK_PIN, 1);
    }
    else{
        ESP_LOGI(DOOR_RELAY_LOG_TAG, "relayLock ON (DOOR LOCK)...");
        gpio_set_level(RELAY_LOCK_PIN, 0);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        gpio_set_level(RELAY_LOCK_PIN, 1);
    }
    stateLockCounter++;
    toggleState = !toggleState;
    ESP_LOGI(DOOR_RELAY_LOG_TAG, "State Lock Counter Internal %d", stateLockCounter);
}