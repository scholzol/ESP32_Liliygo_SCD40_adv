# ESP32_Lilygo_SCD40

This is a repo for a ESP32 Liligo test project structure for VSCode and PlatformIO. I tested the usage of the build in TFT display and an ambient sensor SCD40 (temperature, humidity, CO2).

## Installation

In order to use the code provided in this repository you need to provide the credentials of your access point. Or you can comment out the `#include <credentials.h>` and define the ``ssid`` and ``password`` in the main.cpp.

### Provide your credentials

Create a `credentials.h` file (in the sketch folder directly or in the PlatformIO packages folder).
For Windows, I stored the file in `C:\Users\[UserName]\.platformio\packages\framework-arduinoespressif32\libraries\WiFi\src`.

The text file `credentials.h` looks like:

```c++
#pragma once
const char* ssid = "SSID of your AP";
const char* password = "Password of your AP";
```
