#include "FingerprintManager.h"

FingerprintManager::FingerprintManager(FingerprintSensor *sensor) : _sensor(sensor) {}

bool FingerprintManager::setup(){
    return _sensor -> setup();
}

bool FingerprintManager::addFingerprintModel(int id) {
    return _sensor -> addFingerprintModel(id);
}

bool FingerprintManager::deleteFingerprintModel(int id) {
    return _sensor -> deleteFingerprintModel(id);
}