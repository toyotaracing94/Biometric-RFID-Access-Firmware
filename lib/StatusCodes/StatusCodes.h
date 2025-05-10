#ifndef ERROR_CODE_H
#define ERROR_CODE_H

/**
 * @enum ErrorCode
 * @brief Enum representing error codes for various modules in the system.
 *
 * These error codes indicate specific failure reasons during the execution of 
 * fingerprint, NFC, SD card, queue, document parsing, and server-related operations.
 */
enum ErrorCode {
    /// Fingerprint Error Code (100-200)
    FAILED_TO_CONVERT_IMAGE_TO_FEATURE = -100,              /* Failed to convert the image to Fingerprint feature                                   */
    FAILED_IMAGE_CONVERSION_ERROR = -101,                   /* Failed when conversion image                                                         */
    FAILED_TO_MAKE_FINGERPRINT_MODEL = -102,                /* Failed to create the final Fingerprint model                                         */
    FAILED_TO_STORED_FINGERPRINT_MODEL = -103,              /* Failed to store the final Fingerprint model                                          */
    FAILED_TO_CREATE_VERIFICATION_MODEL = -104,             /* Failed to create the verification before model creation                              */
    FAILED_TO_DELETE_FINGERPRINT_MODEL = -105,              /* Failed to delete a Fingerprint model from sensor                                     */
    FAILED_TO_DELETE_ALL_FINGERPRINT_MODEL = -106,          /* Failed to delete all the fingerprints model from sensor                              */
    FAILED_TO_GET_FINGERPRINT_MODEL_ID = -107,              /* Failed to get the Fingerprint ID associate with model                                */
    FAILED_TO_CONNECT_FINGERPRINT_SENSOR = -108,            /* Failed to connect or find the connected Fingerprint sensor                           */
    FAILED_TO_INTIALIZE_FINGERPRINT = -109,                 /* Failed to initialize or setup the Fingerprint sensor                                 */
    FAILED_TO_REGISTER_FINGERPRINT_NO_NAME = -110,          /* Failed to register fingerprint because no name was provided or the name is invalid   */
    FAILED_TO_DELETE_FINGERPRINT_NO_NAME = -111,            /* Failed to delete fingerprint because no name was provided or the name is invalid     */
    FAILED_TO_DELETE_FINGERPRINT_NO_ID = -112,              /* Failed to delete fingerprint because no valid ID was provided                        */
    FAILED_TO_DELETE_FINGERPRINT_NO_NAME_AND_ID = -113,     /* Failed to delete fingerprint because both name and ID are missing                    */
    FAILED_TO_ADD_FINGERPRINT_MODEL = -114,                 /* Failed to store the fingerprint model to the sensor                                  */
    FAILED_TO_GET_FINGERPRINT_FIRST_CONVERT = -115,         /* Failed to convert the first fingerprint image into template features                 */
    FAILED_TO_GET_FINGERPRINT_SECOND_CONVERT = -116,        /* Failed to convert the second fingerprint image into template features                */
    FINGERPRINT_CAPTURE_TIMEOUT = -117,                     /* Timeout occured while waiting for the FIngerprint Image Input                        */
    NO_FINGERPRINTS_FOUND_UNDER_USER = -118,                /* There are no Fingerprint was found under user                                        */
    FAILED_TO_DELETE_FINGERPRINTS_USER = -119,              /* Failed to delete the fingerprints under user                                         */
    FAILED_DELETING_ALL_FINGERPRINTS_MODEL = -120,          /* Failed to delete all the fingerprints model from the sensor                          */
    FAILED_TO_DELETE_FINGERPRINT_ACCESS_FILE = -121,        /* Failed to delete the Fingerprint key access .json file                               */         

    /// NFC Error Code (201-300)
    FAILED_TO_REGISTER_NFC_NO_NAME = -201,                          /* Failed to register NFC access because no name was provided                           */
    FAILED_TO_DELETE_NFC_NO_NAME = -202,                            /* Failed to delete NFC access because no name was provided                             */
    FAILED_TO_DELETE_NFC_NO_ID = -203,                              /* Failed to delete NFC access because no ID was provided                               */
    FAILED_TO_DELETE_NFC_NO_NAME_AND_ID = -204,                     /* Failed to delete NFC access because both name and ID were not provided               */
    NFC_CARD_TIMEOUT = -205,                                        /* Timeout occurred while waiting for NFC card input                                    */
    FAILED_TO_SEND_REQUEST_TO_SERVER = -206,                        /* Failed to send a request to the backend server                                       */
    FAILED_TO_GET_VISITOR_ID_TO_NFC = -207,                         /* Failed to obtain visitor ID for NFC registration                                     */
    FAILED_TO_PARSE_RESPONSE_SERVER = -208,                         /* Failed to parse the server's response during NFC operation                           */
    FAILED_TO_REGISTER_NFC_NO_VISITOR_ID_OR_KEY_ACCESS_ID = -209,   /* Failed to register NFC access because no Visitor ID or Key Access was provided       */
    FAILED_TO_REGISTER_NFC_NO_NAME_AND_ID = -210,                   /* Failed to register NFC access because no name and ID was provided                    */
    FAILED_TO_DELETE_NFCS_USER = -211,                              /* Failed to delete NFC access under such user                                          */

    /// SD Card Error (301-400)
    FAILED_TO_RETRIEVE_KEYACCESSID_FROM_SDCARD = -301,      /* Failed to retrieve Key Access ID from the SD card                                       */
    FAILED_SAVE_FINGERPRINT_ACCESS_TO_SD_CARD = -302,       /* Failed to save fingerprint access info to SD card                                    */
    FAILED_DELETE_FINGERPRINT_ACCESS_FROM_SD_CARD = -303,   /* Failed to delete fingerprint access info from SD card                                */
    FAILED_SAVE_NFC_ACCESS_TO_SD_CARD = -304,               /* Failed to save NFC access info to SD card                                            */
    FAILED_DELETE_NFC_ACCESS_TO_SD_CARD = -305,             /* Failed to delete NFC access info from SD card                                        */

    /// Queue Related Error (401-500)
    FAILED_TO_SEND_FINGERPRINT_TO_WIFI_QUEUE = -401,        /* Failed to enqueue fingerprint data to WiFi queue                                     */
    FAILED_TO_RECV_FINGERPRINT_FROM_WIFI_QUEUE = -402,      /* Failed to receive fingerprint data from WiFi queue                                   */
    FAILED_TO_SEND_NFC_TO_WIFI_QUEUE = -403,                /* Failed to enqueue NFC data to WiFi queue                                             */
    FAILED_TO_RECV_NFC_FROM_WIFI_QUEUE = -404,              /* Failed to receive NFC data from WiFi queue                                           */

    /// Document Parsing Error (501-600)
    FAILED_TO_PARSE_RESPONSE = -501,                        /* General failure when parsing a document or server response                           */

    /// Server Error (601-700)
    FAILED_REQUEST_TO_SERVER = -601,                        /* Server request failed due to connection or response error                            */
    FAILED_GET_VISITORID_FROM_SERVER = -602,                /* Failed to retrieve a visitor ID from the server                                      */
};

/**
 * @enum SuccessCode
 * @brief Enum representing success status codes for key system operations.
 *
 * This enum includes indicators of successful operations as well as "step-notification"
 * codes used to communicate current progress/status to the client (e.g., BLE notification).
 */
enum SuccessCode {
    /// Fingerprint Success Code (100-200)
    START_REGISTERING_FINGERPRINT_ACCESS = 100,             /* Initiating fingerprint registration process                                          */
    SUCCESS_REGISTERING_FINGERPRINT_ACCESS = 101,           /* Successfully registered fingerprint                                                  */
    SUCCESS_DELETING_FINGERPRINT_ACCESS = 102,              /* Successfully deleted fingerprint access                                              */
    STATUS_FINGERPRINT_STARTED_REGISTERING = 103,           /* Started fingerprint registration process                                             */
    STATUS_FINGERPRINT_PLACE_FIRST_CAPTURE = 104,           /* Prompt to place finger for first fingerprint image capture                           */
    STATUS_FINGERPRINT_PLACE_FIRST_HAS_CAPTURED = 105,      /* First fingerprint image has been successfully captured                               */
    STATUS_FINGERPRINT_FIRST_FEATURE_SUCCESS = 106,         /* First fingerprint image has been converted to features successfully                  */
    STATUS_FINGERPRINT_LIFT_FINGER_FROM_SENSOR = 107,       /* Prompt to lift finger after first capture                                            */
    STATUS_FINGERPRINT_PLACE_SECOND_CAPTURE = 108,          /* Prompt to place finger again for second capture                                      */
    STATUS_FINGERPRINT_REGISTERING_FINGER_MODEL = 109,      /* Creating and storing fingerprint model from captured features                        */
    SUCCESS_DELETING_FINGERPRINTS_USER = 110,               /* Success deleted all the fingerprints on under user                                   */
    SUCCESS_DELETING_ALL_FINGERPRINTS_MODEL = 111,          /* Success deleted all the fingerprints model in the sensor                             */
    SUCCESS_DELETING_FINGERPRINT_ACCESS_FILE = 112,         /* Success deleted the fingerprints key access .json file                               */

    /// NFC Success Code (201-300)
    START_REGISTERING_NFC_CARD_ACCESS = 201,                /* Initiating NFC card registration process                                             */
    READY_FOR_NFC_CARD_INPUT = 202,                         /* System is ready and waiting for NFC card input                                       */
    SUCCESS_REGISTERING_NFC_ACCESS = 203,                   /* Successfully registered NFC access                                                   */
    SUCCESS_DELETING_NFC_ACCESS = 204,                      /* Successfully deleted NFC access                                                      */
    STATUS_NFC_CARD_SUCCESS_READ = 205,                     /* Successfully getting the NFC Card UID From the sensor                                */
    SUCCESS_DELETING_NFCS_USER = 206,                       /* Successfully deleted all the fingerprints on under user                              */
};

#endif // ERROR_CODE_H
