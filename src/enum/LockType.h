#ifndef LOCK_TYPE_H
#define LOCK_TYPE_H

/**
 * @enum LockType
 * @brief Enum representing the type of access control (RFID or Fingerprint).
 * 
 */
enum LockType {
    RFID,           /* The lock input is from RFID Sensor           */
    FINGERPRINT     /* The lock input is from Fingerprint Sensor    */
};

#endif
