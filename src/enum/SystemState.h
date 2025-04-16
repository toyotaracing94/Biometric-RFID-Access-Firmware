#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

/**
 * @enum SystemState
 * @brief Enum representing the type of access control (RFID or Fingerprint).
 *
 */
enum SystemState
{
    RUNNING,            /* The normal state of the system operation                                         */
    ENROLL_FP,          /* The state to change system transtition to register new Fingerprint               */
    ENROLL_RFID,        /* The state to change system transtition to register new RFID                      */
    DELETE_FP,          /* The state to change system transtition to delete Fingerprint access              */
    DELETE_RFID,        /* The state to change system transtition to delete RFID access                     */
    UPDATE_VISITOR      /* The state to change system transtition to sync service between esp32 and titan   */
};



#endif
