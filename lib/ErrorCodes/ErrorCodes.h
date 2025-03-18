#ifndef ERROR_CODE_H
#define ERROR_CODE_H

enum ErrorCode {
    /// Fingerprint Error Code (100-200)

    FAILED_TO_CONVERT_IMAGE_TO_FEATURE          = -100,     /* Failed to convert the image to Fingerprint feature                                   */ 
    FAILED_IMAGE_CONVERSION_ERROR               = -101,     /* Failed when conversion image                                                         */
    FAILED_TO_MAKE_FINGERPRINT_MODEL            = -102,     /* Failed to create the final Fingerprint model                                         */
    FAILED_TO_CREATE_VERIFICATION_MODEL         = -103,     /* Failed to create the verification before model creation                              */
    FAILED_TO_DELETE_FINGERPRINT_MODEL          = -104,     /* Failed to delete a Fingerprint model from sensor                                     */
    FAILED_TO_DELETE_ALL_FINGERPRINT_MODEL      = -105,     /* Failed to delete all the fingerprints model from sensor                              */
    FAILED_TO_GET_FINGERPRINT_MODEL_ID          = -106,     /* Failed to get the Fingerprint ID associate with model                                */
    FAILED_TO_CONNECT_FINGERPRINT_SENSOR        = -107,     /* Failed to connect or find the connected Fingerprint sensor                           */
    FAILED_TO_INTIALIZE_FINGERPRINT             = -108,     /* Failed to initialize or setup the Fingerprint sensor                                 */
    FAILED_TO_REGISTER_FINGERPRINT_NO_NAME      = -109,     /* Failed to register fingerprint because no name was provided or the name is invalid   */
    FAILED_TO_DELETE_FINGERPRINT_NO_NAME        = -110,     /* Failed to delete fingerprint because no name was provided or the name is invalid     */
    FAILED_TO_DELETE_FINGERPRINT_NO_ID          = -111,     /* Failed to delete fingerprint because no valid ID was provided                        */
    FAILED_TO_DELETE_FINGERPRINT_NO_NAME_AND_ID = -112,     /* Failed to delete fingerprint because both name and ID are missing                    */

    /// NFC Error Code (201-300)
    FAILED_TO_REGISTER_NFC_NO_NAME              = -201,     /* Failed to register NFC fingerprint because no name was provided                      */
    FAILED_TO_DELETE_NFC_NO_NAME                = -202,     /* Failed to delete NFC fingerprint because no name was provided                        */
    FAILED_TO_DELETE_NFC_NO_ID                  = -203,     /* Failed to delete NFC fingerprint because no ID was provided                          */
    FAILED_TO_DELETE_NFC_NO_NAME_AND_ID         = -204,     /* Failed to delete NFC fingerprint because both name and ID were not provided          */    

    /// SD Card Error
};

#endif
