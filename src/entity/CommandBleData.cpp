#include "CommandBleData.h"

CommandBleData commandBleData;
CommandBleData::CommandBleData() : _command(nullptr), _name(nullptr), _keyAccess(nullptr) {}
CommandBleData::~CommandBleData() {
    if (_command) free(_command);
    if (_name) free(_name);
    if (_keyAccess) free(_keyAccess);
}

void CommandBleData::setCommand(const char* newCommand) {
    if (_command) free(_command);
    _command = strdup(newCommand);
}

void CommandBleData::setName(const char* newName) {
    if (_name) free(_name);
    _name = strdup(newName);
}

void CommandBleData::setKeyAccess(const char* newKeyAccess) {
    if (_keyAccess) free(_keyAccess);
    _keyAccess = strdup(newKeyAccess);
}

const char* CommandBleData::getCommand() const { return _command; }
const char* CommandBleData::getName() const { return _name; }
const char* CommandBleData::getKeyAccess() const { return _keyAccess; }

void CommandBleData::clear() {
    if (_command) free(_command);
    if (_name) free(_name);
    if (_keyAccess) free(_keyAccess);

    _command = nullptr;
    _name = nullptr;
    _keyAccess = nullptr;
}

// Helper function to duplicate a string (uses malloc)
char* CommandBleData::strdup(const char* str) {
    if (!str) return nullptr;
    size_t len = strlen(str) + 1;
    char* copy = (char*)malloc(len);
    if (copy) memcpy(copy, str, len);
    return copy;
}
