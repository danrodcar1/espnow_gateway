<h1> Autoconfigurable wireless sensor network framework for data fusion at the edge </h1>

![Badge en Desarollo](https://img.shields.io/badge/STATUS-EN%20DESAROLLO-green)
![GitHub Org's stars](https://img.shields.io/github/stars/camilafernanda?style=social)

Class designed in C++ to provide automatic connection to multiple ESP32 devices using ESP-NOW. The programming of this infrastructure allows the automatic and fast connection of multiple devices of the Espressif family with minimum consumption. This infrastructure will allow the study of data acquired by the ADC as well as its data fusion at the edge, focusing on a decentralized paradigm.

## 🔨 Infrastructure functionalities 

- `Automatic pairing`: Thanks to the ESP-NOW connection protocol and using the designed class, we can quickly pair to a gateway or central node, as well as to other devices belonging to our personal area network in order to exchange messages or data.
- `Efficient connectivity to WiFI`: WiFi usage has been reduced to a minimum to save as much energy as possible. We have opted for the radio protocol built into the Espressif devices and have left the WiFi option for other services outside of messaging.
- `Fast and efficient coding`: The code is entirely programmed in C++ and compact in one class, using real-time programming tools (FreeRTOS) to optimize the device's resource savings. This feature facilitates programming since the programmer will only have to add some configuration information and put his code into operation.
- `OTA update capability`: The device is capable of receiving over-the-air updates, since when it receives a request via MQTT for an update, it performs the update routine.

## 📁 Project organization
```
espnow_project/
├── core/
│   ├── inc/
│   │   ├── ADConeshot.h
│   │   ├── AUTOpairing.h
│   │   └── AUTOpairing_common.h
│   ├── src/
│   │   └── main.cpp
│   └── CMakeLists.txt
├── CMakeLists.txt
├── README.md
├── sdkconfig
└── partitions_two_ota.csv
```


The functions that were responsible, for example, for the connection via ESP-NOW to the device that was performing the pass-through function, as well as the configuration or presetting process and the the configuration or presetting process and the OTA update process, have been included in a class within the OTA update process, have been included in a class within the "AUTOpairing.h" library. "AUTOpairing.h". On the other hand, the operation of the ADC converter has been modeled in the "ADC.h" library.
modeled in the "ADConeshot.h" library. The structure is simpler and allows
the code in different classes that allow to model the behavior of the device in a more efficient way.
the behavior of the device in a more efficient way.

* The "AUTOpairing.h" library has been designed to host a class that composes the operation of the autopairing of the the device to be connected to the gateway and the functions that model the configuration and connection to the the configuration and connection to the server that hosts the file to perform the FOTA update.

* The "ADConeshot.h" library models the behavior of the analog-to-digital converter. It allows to be configured, select the channel and various parameters depending on the needs that are required. 

This modeling in classes allows, in short, to modular and simpler code, whose functionality is reduced exclusively to what it is intended to do. functionality is reduced exclusively to what is intended to be done. The functional code is hidden from the user, so that the user is only concerned with configuring the different functional the different functional parameters required.

## ✔️ Technologies used in the project

* [VSCode](https://code.visualstudio.com/) - Programming framework used
* [NODE-RED](https://nodered.org/) - Cloud data manager
* [ESP32-C3](https://www.espressif.com/en/products/socs/esp32-c3) - Microcontroller used for testing
