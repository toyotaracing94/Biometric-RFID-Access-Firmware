#ifndef COMMAND_BLE_DATA_H
#define COMMAND_BLE_DATA_H
#include <cstring> 

// TODO : Find a better way perhaps to move this data to main thread loop rather using malloc 
class CommandBleData {
public:
    CommandBleData();
    ~CommandBleData();

    // Setters
    void setCommand(const char* newCommand);
    void setName(const char* newName);
    void setKeyAccess(const char* newKeyAccess);

    // Getters
    const char* getCommand() const;
    const char* getName() const;
    const char* getKeyAccess() const;

    // Clear/reset values
    void clear();

private:
    char* _command;
    char* _name;
    char* _keyAccess;

    // Helper function to duplicate a string (uses malloc)
    char* strdup(const char* str);
};

extern CommandBleData commandBleData;

#endif
