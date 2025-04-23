#ifndef ERROR_CODE_H
#define ERROR_CODE_H

enum ErrorCode {
    /// Fingerprint Error Code (100-200)
    FAILED_TO_CONVERT_IMAGE_TO_FEATURE = -100,          /* Failed to convert the image to Fingerprint feature                                   */
    FAILED_IMAGE_CONVERSION_ERROR = -101,               /* Failed when conversion image                                                         */
    FAILED_TO_MAKE_FINGERPRINT_MODEL = -102,            /* Failed to create the final Fingerprint model                                         */
    FAILED_TO_CREATE_VERIFICATION_MODEL = -103,         /* Failed to create the verification before model creation                              */
    FAILED_TO_DELETE_FINGERPRINT_MODEL = -104,          /* Failed to delete a Fingerprint model from sensor                                     */
    FAILED_TO_DELETE_ALL_FINGERPRINT_MODEL = -105,      /* Failed to delete all the fingerprints model from sensor                              */
    FAILED_TO_GET_FINGERPRINT_MODEL_ID = -106,          /* Failed to get the Fingerprint ID associate with model                                */
    FAILED_TO_CONNECT_FINGERPRINT_SENSOR = -107,        /* Failed to connect or find the connected Fingerprint sensor                           */
    FAILED_TO_INTIALIZE_FINGERPRINT = -108,             /* Failed to initialize or setup the Fingerprint sensor                                 */
    FAILED_TO_REGISTER_FINGERPRINT_NO_NAME = -109,      /* Failed to register fingerprint because no name was provided or the name is invalid   */
    FAILED_TO_DELETE_FINGERPRINT_NO_NAME = -110,        /* Failed to delete fingerprint because no name was provided or the name is invalid     */
    FAILED_TO_DELETE_FINGERPRINT_NO_ID = -111,          /* Failed to delete fingerprint because no valid ID was provided                        */
    FAILED_TO_DELETE_FINGERPRINT_NO_NAME_AND_ID = -112, /* Failed to delete fingerprint because both name and ID are missing                    */
    FAILED_TO_ADD_FINGERPRINT_MODEL = -113,

    /// NFC Error Code (201-300)
    FAILED_TO_REGISTER_NFC_NO_NAME = -201,              /* Failed to register NFC fingerprint because no name was provided                      */
    FAILED_TO_DELETE_NFC_NO_NAME = -202,                /* Failed to delete NFC fingerprint because no name was provided                        */
    FAILED_TO_DELETE_NFC_NO_ID = -203,                  /* Failed to delete NFC fingerprint because no ID was provided                          */
    FAILED_TO_DELETE_NFC_NO_NAME_AND_ID = -204,         /* Failed to delete NFC fingerprint because both name and ID were not provided          */
    NFC_CARD_TIMEOUT = -205, 
    FAILED_TO_SEND_REQUEST_TO_SERVER = -206,
    FAILED_TO_GET_VISITOR_ID_TO_NFC = -207,
    FAILED_TO_PARSE_RESPONSE_SERVER = -208,

    /// SD Card Error (301-400)
    FAILED_TO_RETRIEVE_VISITORID_FROM_SDCARD = -301,
    FAILED_SAVE_FINGERPRINT_ACCESS_TO_SD_CARD = -302,
    FAILED_DELETE_FINGERPRINT_ACCESS_FROM_SD_CARD = -303,

    /// Queue Related Error (401-500)
    FAILED_TO_SEND_FINGERPRINT_TO_WIFI_QUEUE = -401,
    FAILED_TO_RECV_FINGERPRINT_FROM_WIFI_QUEUE = -402,

    /// Document Parsing Error (501-600)
    FAILED_TO_PARSE_RESPONSE = -501,

    /// Server Error (601-700)
    FAILED_REQUEST_TO_SERVER =  -601,
    FAILED_GET_VISITORID_FROM_SERVER = -602,

};


enum SuccessCode {
    /// Fingerprint Success Code (100-200)
    START_REGISTERING_FINGERPRINT_ACCESS = 100,
    SUCCESS_REGISTERING_FINGERPRINT_ACCESS = 101,
    SUCCESS_DELETING_FINGERPRINT_ACCESS = 102,

    /// NFC Success Code (201-300)
    REGISTERING_NFC_CARD_ACCESS = 200,
    NFC_WAITING_FOR_CARD = 220,
};

#endif
