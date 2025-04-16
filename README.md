# Biometric RFID Access Firmware
This is repo for holding the code for Firmware of Group C Project at Capability Center Division on Toyota Motor Indonesia which is Door Locking Mechanism Firmware on ESP32. The project focuses on a door locking mechanism that utilizes an ESP32 microcontroller along with Fingerprint and NFC sensors for access control, where it stored the data access in external SD Card. This project is using PlatformIO to deliver this project.

## Why PlatformIO not ESP-IDF Framework
It's also been a question to ask for myself when I first start this project, just why do this? Previously if you see in previous version, you'll will see that we are using Arduino IDE to develop the program (now it's been archived). Of course it's not a problem if we just using this as a toy project or something like that, Arduino IDE environment really hide the ugly truth behind how to config the microcontroller for us(mostly Arduino and Espressif) for us to use with ease. We can simply just plug and play, and can start more actually care about the program. But as the feature keep adding, this thing become harder to maintain. Just think about it, all the functions are in the same place of this one big `ino` files, making things harder, as if we change some of the function, it will introduce also a new breaking changes to the rest of the flow of the program.

So I know I want to make this program code to become more modular and easier to maintain, if this program could get bigger in the future. I was also hoping this project could become as a  reference to other project on about how to thinking more about of more proper way of firmware development. As goes by, my option for development goes by PlatformIO and ESP-IDF. ESP-IDF is the official works provided by the Esspresif System, the manufacturer of ESP32 on how we can develop the firmare code that is using the ESP32 Microcontroller family system, while PlatformIO is a cross-platform, cross-architecture, multiple framework, professional tool for embedded systems engineers and for software developers who write applications for embedded products. It supports many different software development kits, platform where ESP-IDF is included to the PlatformIO development support. In the end, I choose PlatformIO instead of ESP-IDF, the point of why is reason below, but still it's really a relieve to move away from the Arduino IDE environment.

| Problems                    | PlatformIO                                            | ESP-IDF                                                      |
| :-------------------------- | :---------------------------------------------------- | :----------------------------------------------------------- |
| **Installation**            | Easy installation via VS Code extension               | ALso easy installation via VS Code extension                 |
| **Modern Approach**         | Offers a more integrated, user-friendly experience with Unit Test incoporation  | More extra effor for manual configuration especially their manual cpp intellisense                        |
| **Good Structure**          | Supports multi-platform projects and integrations     | Provides detailed control over hardware configuration        |
| **Arduino Library Support** | One of the big reasons, many on this project stilluse 3rd Party Libraries for the sensor, where in PlatformIO, we can hack this by adding more framework to project configurations                      | Limited Arduino compatibility; more custom solutions needed which is will slow down development  |

## Install
To build this project, you need [PlatformIO](https://platformio.org/) installed.

It is recommended to use Visual Studio Code with the PlatformIO plugin for development.  
[Visual Studio Code](https://code.visualstudio.com/download)  
[PlatformIO vscode plugin](https://platformio.org/install/ide?install=vscode)

I recommend watching this [video](https://www.youtube.com/watch?v=tc3Qnf79Ny8&t=187s) to start your journey to understand how to start and installing the necessary tools for developing Embedded System Firmware using PlatformIO.

## Project Structure
In this project, I divide the project into 3 main structure, the source code that relates to the bussiness action operation, the lib static source code, and the unit test project

```
Biometric-RFID-Access-Firmware/
├── lib/
│   ├── Fingerprint/
│   │   ├── Fingerprint.h
│   │   ├── Fingerprint.cpp
│   ├── NFC/
│   │   ├── NFC.h
│   │   ├── NFC.cpp
│   ├── ....
├── src/
│   ├── enum
│   │   ├── MyEnum.h
│   ├── service
│   │   ├── service.h
│   ├── ....
│   ├── main.cpp
├── test/
├── custom_partitions.csv
├── platformio.ini
```
As for the explanation
- `lib`:  
This folder will hold the most of the sensor implementation and configurations of the project, where criteria for putting in here is so it can be use into another project  
- `src`:  
This folder will hold service and also the entry point of the firmware program, such as services to wrap the code of the sensor, or the task creation and management  
- `test`:  
This folder will hold the unit test of the program. Why I choose PlatformIO is that they have many support for many Unit Test Framework for C++ Software development
- `custom_partitions.csv`:  
This file contains the configurations for the partitions table to be load into the ESP32 flash
- `platformio.ini`:  
This file is a configuration to set up your development environment, such as the frameworks, platform we use, the 3rd party libraries, board builds configurations, etc.

## Project Architecture
So, in this project more or less I designed the project with the believe of what I borrow from the Software development common architecture which is [a CLEAN architecture](https://blog.cleancoder.com/uncle-bob/2012/08/13/the-clean-architecture.html) and OOP Design, that's why I choose to make the firmware in C++ environment rather plain C as C++ support classes, opposite of [many disucssing forum](https://stackoverflow.com/questions/812717/is-there-any-reason-to-use-c-instead-of-c-for-embedded-development) says that developing firmware in C are rather the norm rather in C++.
This project is structured with modularity and scalability in mind. The system is divided into three main layers:

<p align="center">
  <img width="69%"src="docs/images/Firmware Architecture.png">
</p>

So in detailed explanation
1. Modules (Sensors, Protocols, etc.) --> All core components such as sensors, actuators, and communication protocols are implemented as independent C++ classes. These are self-contained and reusable across the system. By isolating hardware interaction into its own folder or module, we ensure that each hardware or protocol can be tested independently. This makes our code becomes cleaner and easier to maintain or swap out. Everything at this level can be seen as building blocks for higher-level actions.
2. Services --> Services act as the middle layer that brings together multiple modules to perform real-world actions. For example, unlocking a door with an NFC card would require An NFC reader module to detect the card, actuator module to unlock the door and a SD card module to log the event. So, Instead of handling all these in one place, a Service encapsulates the logic of "unlocking a door" by coordinating the relevant modules. This makes high-level actions clean, readable, and easier to test.
3. Tasks --> When a Service becomes complex or needs to operate with its own logic, lifecycle, or state machine (e.g., periodic checking, event-based execution), it can be promoted into a Task. A Task is essentially a Service that runs independently—either as a FreeRTOS task, a separate thread, or run with its own loop and timing mechanisms. This keeps the system responsive and decouples critical behaviors from one another. This is critical let say that one service is need to send something to the server, so from this we need a module to connect to the WiFi. But, as WiFi really have many exsternal disturbance, so if we incldue WiFi as a procedure in a let's say Service 'A', then if WiFi suddenly disconnect, it will disturb the Service 'A', that's why it's easier to decouple WiFi in it's own little world.

## How to Build and Flash
<p align="center">
  <img width="79%"src="docs/images/PlatformIO Tools.png">
</p>

To build(compile/verify) and flash(upload) the code to our ESP32, there are two ways, by using the PlatformIO extensions that already provided to use when we installed them or in terminal. I will only show it using by what is already provided to use


If using the PlatformIO terminal/CLI, the list of them are [here](https://docs.platformio.org/en/latest/core/userguide/index.html#commands).

### Build/Compile
When want to compile the project first to build it's binaries, we can hit up the `tick` or (✓) mark, or if using PlatfromIO CLI, we can build or project by this command
```sh
pio run
```

### Flash
When want to flash the binaries of the code into the ESP32, click the `arrow` right sign or (→) mark. After that, plug your device and select the port of the device, by default PlatformIO will automatically select the device port based on the current active one. But if you want to manually select the port device, you can click the `power cord` mark in the bottom right corner.  Or if you want to using PlatformIO terminal CLI to flash, you can use
```sh
pio run --target upload 
```

or 
```sh
pio run --upload-port SELECTED-COM-PORT --target upload
```

### Monitor
We can also monitor the ESP32 MCU by read the program on the UART port (USB port). In Arduino IDE this named Serial Monitor. We can hit up the `power cord` mark, or if using PlatfromIO CLI, we can monitor the device by this command
```sh
pio device monitor
```
or 
```sh
pio device monitor --upload-port SELECTED-COM-PORT
```

## Library Dependencies
For this project, we use several 3rd Party libraries to make this code functional, we can install them by searching them in the PlatformIO libraries
* [ArduinoJson](https://github.com/bblanchon/ArduinoJson)
* [Adafruit_PN532](https://github.com/adafruit/Adafruit-PN532)
* [DFRobot_ID809_I2C](https://github.com/DFRobot/DFRobot_ID809_I2C)
* [Adafruit Fingerprint Sensor Library](https://github.com/adafruit/Adafruit-Fingerprint-Sensor-Library)


## Possible Development
There are possible development to increase the usability and the security of this project  
1. Is it possible to add a feature to Lock/Unlock the Car by using Smartphone but instead of pyshcially press the button to Lock/Unlock the Car, but what if using Proximity feature to have the abillity by allowing seamless access to the vehicle without the need for physical action
2. Adding a way to safely hash the Key Access (wether its the Fingerprint or the NFC UID) in the SD Card, so if somehow someone have access to the physical SD Card, they cannot know the content of it
3. Refactor the BLE Code to use the ESP-IDF BLE API rather using the BLE API that was provided by espressif-arduino or by Kolban, as the code is rather too big

## Author
### Firmware Development
<a id="1">[1]</a> Muhammad Juniarto (CC Batch 3 Members)  
<a id="2">[2]</a> Dhimas Dwi Cahyo  (CC Batch 2 Members)  
<a id="3">[3]</a> Ahmadhani Fajar Aditama (AKTI Members)

## Project Reference
1. https://github.com/AllanOricil/esp32-mfa-authenticator
2. https://github.com/UtilitechAS/amsreader-firmware
