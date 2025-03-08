/**
  * @brief Configuration for the Hardware Serial in Arduino Config
*/
#define __hardwareSerial 2          /*!< Relay Unlock                       */

/**
  * @brief Configuration of Pin for Lock and Unlock of Relay
*/
#define __relayUnlock 25            /*!< Relay Unlock                       */
#define __relayLock 26              /*!< Relay Lock                         */

/**
  * @brief Configuration of Pin I2C CONFIGURATION FOR RFID SENSOR
*/
#define __SDA_PIN 21                /*!< Pin SDA to PN532                   */  
#define __SCL_PIN 22                /*!< Pin SCL to PN532                   */

/**
  * @brief Configuration of SPI PIN CONFIGURATION FOR MEMORY CARD MODULE
*/
#define __CS_PIN 5                  /*!< Chip Select pin                     */ 
#define __SCK_PIN 18                /*!< Clock pin                           */ 
#define __MISO_PIN 19               /*!< Master In Slave Out                 */ 
#define __MOSI_PIN 23               /*!< Master Out Slave In                 */ 

/**
  * @brief Configuration for the Data RFID and Fingerprints File Data in SD Card 
*/
#define FINGERPRINT_FILE_PATH "/fingerprints.json"
#define RFID_FILE_PATH "/rfid.json"