#ifndef QUEUE_MESSAGE_H
#define QUEUE_MESSAGE_H

#include "Arduino.h"
#include "enum/OperationState.h"

struct NFCQueueRequest {
    int request_id;
    char username[25];
    char visitorId[40];
    char uidCard[32]; 
    char vehicleInformationNumber[24];
    OperationState state;
    int statusCode;
};

struct NFCQueueResponse {
    int request_id;
    char response[512];
};

struct FingerprintQueueRequest {
    int request_id;
    int fingerprintId;
    char username[25];
    char visitorId[40];
    char vehicleInformationNumber[24];
    OperationState state;
    int statusCode;
};

struct FingerprintQueueResponse{  
    int request_id;
    char response[512];
};

#endif