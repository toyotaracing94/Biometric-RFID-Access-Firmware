#include "CommandBleData.h"

CommandBleData commandBleData;
CommandBleData::CommandBleData() : _command(nullptr), _name(nullptr), _keyAccess(nullptr), _visitorId(nullptr) {}
CommandBleData::~CommandBleData()
{
    if (_command)
        free(_command);
    if (_name)
        free(_name);
    if (_keyAccess)
        free(_keyAccess);
    if (_visitorId)
        free(_visitorId);
}

void CommandBleData::setCommand(const char *newCommand)
{
    if (_command)
        free(_command);
    _command = strdup(newCommand);
}

void CommandBleData::setName(const char *newName)
{
    if (_name)
        free(_name);
    _name = strdup(newName);
}

void CommandBleData::setKeyAccess(const char *newKeyAccess)
{
    if (_keyAccess)
        free(_keyAccess);
    _keyAccess = strdup(newKeyAccess);
}

void CommandBleData::setVisitorId(const char *newVisitorId)
{
    if (_visitorId)
        free(_visitorId);
    _visitorId = strdup(newVisitorId);
}

const char *CommandBleData::getCommand() const { return _command; }
const char *CommandBleData::getName() const { return _name; }
const char *CommandBleData::getKeyAccess() const { return _keyAccess; }
const char *CommandBleData::getVisitorId() const { return _visitorId; }

void CommandBleData::clear()
{
    if (_command)
        free(_command);
    if (_name)
        free(_name);
    if (_keyAccess)
        free(_keyAccess);
    if (_visitorId)
        free(_visitorId);

    _command = nullptr;
    _name = nullptr;
    _visitorId = nullptr;
    _keyAccess = nullptr;
}

// Helper function to duplicate a string (uses malloc)
char *CommandBleData::strdup(const char *str)
{
    if (!str)
        return nullptr;
    size_t len = strlen(str) + 1;
    char *copy = (char *)malloc(len);
    if (copy)
        memcpy(copy, str, len);
    return copy;
}
