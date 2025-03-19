# Biometric-RFID-Access-Firmware
This is repo for holding the Firmware of Group C at Capability Center Division on Toyota Motor Indonesia which is Door Locking Mechanism Firmware on ESP32

## How to Run
Clone the repository here and move to the directory. For this project, we use several 3rd Party libraries to make this code functional. Most of them used for the sensor on the firwmare. So install them first The list of them is  
* [ArduinoJson](https://github.com/bblanchon/ArduinoJson)
* [Adafruit_PN532](https://github.com/adafruit/Adafruit-PN532)
* [DFRobot_ID809_I2C](https://github.com/DFRobot/DFRobot_ID809_I2C)
* [SD_Card](https://docs.arduino.cc/libraries/sd/) -- We are using SD Card by Arduino rather the ESP32

and after that, then simply open the `.ino` files using Arduino IDE and then try to verify/compile them first to see if everything works fine. If none whatsoever going wrong, you can then try to flash/upload them into microcontroller.

## What can you Changed
Because right now, the development we are using is this big `ino` file, so you can change what inside of the `.ino` files. Most of the function as well is still being held as an individual function rather in one class, so do note that changing one of the function will affects the rest of them. So right now, to make everything looks clean, when you want to create new function, it's proper if you initialize the function first at the top of the program line, and then implement them at the bottom. 


## Future Plan
Will move the development framework using PlatformIO for better maintainability