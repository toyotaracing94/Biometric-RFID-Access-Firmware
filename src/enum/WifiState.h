#ifndef WIFI_STATE_H
#define WIFI_STATE_H


/**
 * @enum WifiState
 * @brief Enum representing the state of WiFi connection
 * 
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
