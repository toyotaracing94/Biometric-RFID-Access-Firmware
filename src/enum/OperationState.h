#ifndef OPERATION_STATE_H
#define OPERATION_STATE_H

/**
 * @enum OperationState
 * @brief Enum representing the type of operation control (RFID or Fingerprint).
 *
 */
enum OperationState
{
    ADD_FP,             /* Operation state when register new Fingerprint Access                 */
    ADD_RFID,           /* Operation state when register new RFID Card Access                   */
    REMOVE_FP,          /* Operation state when removing Fingerprint access from the system     */
    REMOVE_RFID,        /* Operation state when removing RFID Access from the system            */
    AUTH_FP,            /* Operation state when authenticate the Fingerprint Access             */
    AUTH_RFID,          /* Operation state when authenticate the RFID Card Access               */
};


#endif
