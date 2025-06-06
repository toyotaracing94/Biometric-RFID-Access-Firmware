#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

/**
 * @enum SystemState
 * @brief Enum representing the type of access control (RFID or Fingerprint).
 *
 */
enum SystemState{
    RUNNING,            /* The normal state of the system operation                                             */
    ENROLL_FP,          /* The state to change system transtition to register new Fingerprint                   */
    DELETE_FP,          /* The state to change system transtition to delete Fingerprint access                  */
    DELETE_FP_USER,     /* The state to change the system transitition to delete a user fingerprint access      */
    DELETE_FP_SENSOR,   /* The state to change the system transitition to delete all fingerprint model sensor   */
    DELETE_FP_FS,       /* The state to change the system transitition to delete fingerprint key access .json   */
    AUTHENTICATE_FP,    /* The state when authenticate the Fingerprint Access                                   */
    ENROLL_RFID,        /* The state to change system transtition to register new RFID                          */
    DELETE_RFID,        /* The state to change system transtition to delete RFID access                         */
    DELETE_RFID_USER,   /* The state to change the system transtition to delete a user RFID access              */
    DELETE_RFID_FS,     /* The state to change the system transitition to delete RFID key access .json          */
    AUTHENTICATE_RFID,  /* The state when authenticate the RFID Card Access                                     */
    DELETE_ACCESS_USER, /* The state to change system transtition to delete all key access under a user id      */
    UPDATE_VISITOR,     /* The state to change system transtition to sync service between esp32 and titan       */
    DOOR_LOCK,          /* The state where the door is locked through a relay or other mechanism.               */
    DOOR_UNLOCK,        /* The state where the door is unlocked, allowing access.                               */
};



#endif
