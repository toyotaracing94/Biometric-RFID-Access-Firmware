#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

enum SystemState {
    RUNNING,
    ENROLL_FP,
    ENROLL_RFID,
    DELETE_FP,
    DELETE_RFID,
    GETTING_FP,
    GETTING_RFID,
    SYNC_VISITOR
};

#endif
