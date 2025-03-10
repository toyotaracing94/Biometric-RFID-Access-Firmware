#define LOG_TAG "DOOR_RELAY"
#include <esp_log.h>
#include "DoorRelay.h"

DoorRelay::DoorRelay(){
    setup();
}

bool DoorRelay::setup() {
    if (gpio_set_direction(RELAY_UNLOCK_PIN, GPIO_MODE_OUTPUT) != ESP_OK) {
        return false;
    }
    if (gpio_set_direction(RELAY_LOCK_PIN, GPIO_MODE_OUTPUT) != ESP_OK) {
        return false;
    }
    if (gpio_set_level(RELAY_UNLOCK_PIN, 1) != ESP_OK) {
        return false;
    }
    if (gpio_set_level(RELAY_LOCK_PIN, 1) != ESP_OK) {
        return false;
    }
    return true;
}

void DoorRelay::toggleRelay(){
    if (toggleState) {
        ESP_LOGI(LOG_TAG, "relayUnlock ON (DOOR UNLOCK)...");
        gpio_set_level(RELAY_UNLOCK_PIN, 0);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        gpio_set_level(RELAY_UNLOCK_PIN, 1);
    } else {
        ESP_LOGI(LOG_TAG, "relayLock ON (DOOR LOCK)...");
        gpio_set_level(RELAY_LOCK_PIN, 0);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        gpio_set_level(RELAY_LOCK_PIN, 1);
    }
    stateLockCounter++;
    toggleState = !toggleState;
    ESP_LOGD(LOG_TAG, "State Lock Counter Internal %d", stateLockCounter);
}