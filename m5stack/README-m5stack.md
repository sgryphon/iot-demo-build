M5 Stack
========

Website: https://m5stack.com/

Core / Fire / Core2 / Core2 AWS: Stackable modules with screen and buttons, and lots of options to expand and add sensors. (5cm x 5cm)

Atom: Miniature devices that are just the controller, with a range of add-on sensors available. (2.5cm x 2.5cm)

Stick: Full feature miniature devices.

Hardware - Core 2
-----------------

Chip: ESP32-D0WDQ6-V3
Flash: 16MB
PSRAM: 8MB
Interfaces: 1x TypeC, 1x Grove (I2C+I/O+UART)
Screen: 320x240 LCD
Input: touch screen, 3 virtual buttons, power, reset
Speaker: 1W
LED: Green power indicator
Vibration
Microphone
6-axis Intertial Measurement Unit
RTC
390mAh battery

Base has 2x Grove (B, C), and 2x LED

Programming options
-------------------

* Arduino framework
* FreeRTOS
* .net Nano framework (on FreeRTOS)
* MicroPython
* Other ESP32 platforms, e.g. ESPHome


### Arduino - with Core2 for AWS

* Quick start: https://docs.m5stack.com/en/quick_start/core2_for_aws/arduino

* Basic API: https://docs.m5stack.com/en/api/core2/system

* Extended examples: https://github.com/m5stack/M5Core2/tree/master/examples

Arduino is not just a family of devices but a conventient programming framework that has been extended to many other boards beyond the original, including the M5Stack boards (which use an ESP32 chip).

There is also an Arduino IDE, which is simple to use with a single 'sketch' file for basic programs, however it has limitations -- no integrated source control, no debugging, and libraries are shared across the whole IDE (makes things simpler, but leads to problems with multiple projects).

#### PlatformIO

PlatformIO is a plugin for popular editors, including VS Code, that supports the Arduino framework (and more). As well as source control and debugging, this allows libraries to be controlled per project -- making the setup a bit more complicated, but a lot more flexible.

**Quick start**

Follow the PlatformIO instructions for installion, for example in VS Code it is available as an extension that you can add to your editor (you may also need to install Python3).

With PlatformIO installed you can create a new project, using a custom location if you want, called m5core2_hello (or whatever your version is).

Select Board = M5Stack Core2, and Framework = Arduino, then click Create.

You can also do this using the PlatformIO CLI tool, e.g. from the PlatformIO Core CLI console within the VS Code plugin (or outside it, if you have installed PlatformIO Core on your base machine).

```shell
mkdir m5core2_hello
cd m5core2_hello
pio project init --board m5stack-core2
```

PlatformIO will install all the toolchains needed for you.

You can also initialise a Git repository in the folder (if you don't already have one), to keep a track of changes:

```shell
git init
git add .
git commit -m "Initial PlatformIO M5Core2 empty project"
```

You then need to go to PlatformIO > Libraries and install the M5Core2 library from M5Stack.

Or from the CLI:

```shell
pio pkg install --library M5Core2
```

You then need to create a `src/main.cpp` file (if using the IDE new project a file may already have been created) with a simple Hello World example:

```cpp
#include <Arduino.h>
#include <M5Core2.h>

void setup() {
  M5.begin();
  M5.Lcd.print("Hello M5Stack Core2\n(with Arduino framework)\nvia PlatformIO");
}

void loop() {
}
```

To build you can either use the IDE menu, or the CLI (the "run" command runs the build, not the project):

```powershell
pio run
```

To deploy (upload) to your device:

```powershell
pio run --target upload
```

In Git you can see the changes to the libraries and configuration changes that you have made. Slightly more complicated than the Arduino IDE as there are ~5 project files (instead of one) and a few extra directories and instructions. This is to support different libraries per project, as well as multi-board targetting, and other features.

**Wifi**

`m5core2_wifi` has Advanced / Wifi example from https://github.com/m5stack/M5Core2/tree/master/examples



M5Stack Core2 to Azure (via WiFi)
---------------------------------

Create a basic M5Core2 project in PlatformIO:

```shell
mkdir m5core2_azureiot
cd m5core2_azureiot
pio project init --board m5stack-core2
pio pkg install --library M5Core2
pio pkg install --library "azure/Azure SDK for C"
```

To build (`pio run`) I had to make sure the lib_deps section had just "azure/Azure SDK for C" (without any version).

Note that the Arduino (used by PlatformIO) port of the library is version 1.0.0-beta.5, which corresponds to the library version 1.3.2. PlatformIO registry information: https://registry.platformio.org/libraries/azure/Azure%20SDK%20for%20C

Arduino port source code (with samples): https://github.com/Azure/azure-sdk-for-c-arduino

Note that the example is an Arduino IDE sketch file (.ino) and uses some Espressif libraries for Wifi & MQTT, whereas we use M5.


Library references
------------------

PlatformIO library registry: https://registry.platformio.org/

* M5Core2 library: https://registry.platformio.org/libraries/m5stack/M5Core2
* AWSLabs SimpleIoT: https://github.com/awslabs/simpleiot-arduino

* Arduino-ESP32 IPv6 example: https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/examples/WiFiIPv6/WiFiIPv6.ino

https://github.com/khoih-prog/ESP_WiFiManager


* Video on ESP32 IPv6, https://www.youtube.com/watch?v=mn0imqzramQ
  - CONFIG_IPV6_DHCP6_OTHER_FLAGS => "-DLWIP_IPV6_DHCP6=1"
  - CONFIG_IPV6_RA_RDNSS => "-DLWIP_ND6_RDNSS_MAX_DNS_SERVERS=1"
  - CONFIG_IPV6_GLOBAL_AUTOCONFIG => ?
  -  => ESP_IPV6_AUTOCONFIG
  - See: https://github.com/espressif/arduino-esp32/blob/master/tools/sdk/esp32c3/include/lwip/lwip/src/include/lwip/opt.h


TODO
----

core2_azure_simfactory

core2_nbiot_simfactory

core2_azure_factory


