#ifndef WIFI_STATE_H
#define WIFI_STATE_H


/**
 * @enum WifiState
 * @brief Enum representing the state of WiFi connection
 * 
 * For now this is still not being used properly, will try to hold this to better make use of it
 */
enum WifiState {
    NOT_INITIALIZED,
    INITIALIZED,
    WAITING_FOR_CREDENTIALS,
    READY_TO_CONNECT,
    CONNECTING,
    WAITING_FOR_IP,
    CONNECTED,
    DISCONNECTED,
    ERROR
};

#endif
