#ifndef QUEUE_MESSAGE_H
#define QUEUE_MESSAGE_H

#include "Arduino.h"
#include "enum/SystemState.h"

struct nfcQueueMessage {
    String username;
    String uidCard;
    SystemState operation;
    int statusCode;
};

struct fingerprintQueueMessage {
    String username;
    int fingerprintId;
    SystemState operation;
    int statusCode;
};

#endif