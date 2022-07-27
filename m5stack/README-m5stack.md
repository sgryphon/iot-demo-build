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


Arduino - with Core2 for AWS
----------------------------

M5 Stack specific documentation (has the `M5` code and some extensions):

* Quick start: https://docs.m5stack.com/en/quick_start/core2_for_aws/arduino
* Basic API: https://docs.m5stack.com/en/api/core2/system
* Extended examples: https://github.com/m5stack/M5Core2/tree/master/examples

Arduino-ESP32 references (has framework items such as `WiFi`, `I2C`, etc):

* Documentation: https://docs.espressif.com/projects/arduino-esp32/en/latest/index.html
* Library code: https://github.com/espressif/arduino-esp32

There are also third party libraries, e.g. `PubSubClient` for MQTT.

Arduino is not just a family of devices but a conventient programming framework that has been extended to many other boards beyond the original, including the M5Stack boards (which use an ESP32 chip).

There is also an Arduino IDE, which is simple to use with a single 'sketch' file for basic programs, however it has limitations -- no integrated source control, no debugging, and libraries are shared across the whole IDE (makes things simpler, but leads to problems with multiple projects).

**PlatformIO**

PlatformIO is a plugin for popular editors, including VS Code, that supports the Arduino framework (and more). As well as source control and debugging, this allows libraries to be controlled per project -- making the setup a bit more complicated, but a lot more flexible.

The examples use PlatformIO, with the general config of the relevant M5Stack board on the ESP32 platform with the Arduino framework; as well as the arduino-esp32 framework, it includes the M5Stack libraries.

```
platform = espressif32
board = m5stack-core2
framework = arduino
lib_deps = m5stack/M5Core2@^0.1.3
```

### Quick start

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

### Logging

The LCD screen is 320x240, with 0,0 in the top left.

Default text (size 1) fits 53 characters across, and 30 lines (6x8 per character). Size 2 fits 26 characters across, with 15 lines (12x16 per character). Size 3 fits 17 characters across, with 10 lines (18x24 per character).

There is no scrolling, but you can check before writing if the cursor Y position is below the bottom and clear the screen.

To view the device log there is a serial monitor; make sure to set the baud to match what you have defined in the code with `Serial.begin(115200);`.

```shell
pio device monitor --baud 115200
```

There is not a lot of documentation on logging. The Arduino-ESP32 framework redefines and simplifies the base ESP32, so that instead of tags that can be set to different levels at runtime, all logging is controlled by a compile-time flag (and other logging not even compiled in).

The log level is controlled by the build flag, e.g.:

```
build_flags =
    '-D CORE_DEBUG_LEVEL=ARDUHAL_LOG_LEVEL_INFO'
```

You can use the base ESP32 logging macros, `ESP_LOGE`, `ESP_LOGW`, `ESP_LOGI`, `ESP_LOGD`, and `ESP_LOGV`, but note that Arduino-ESP32 has simplified logging with only a single level that cannot be controlled per-tag (but tags are still a good idea). 

```cpp
static const char* TAG = "demo";
...
ESP_LOGD(TAG, "Debug message");
```

The ESP32 macros are forwarded to the Arduino-ESP32 macros (`log_e`, `log_w`, `log_i`, `log_d`, and `log_v`), which are controlled by the `CORE_DEBUG_LEVEL` setting.

The rest of the ESP32 logging settings (`CONFIG_LOG_DEFAULT_LEVEL`, `CONFIG_LOG_MAXIMUM_LEVEL`, and the runtime `esp_log-level_set()`) are ignored and not used.

The details are in the source code: https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-log.h

For more about ESP32 logging (mostly not used), see: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html

Also of relevance: https://cpp4arduino.com/2020/02/07/how-to-format-strings-without-the-string-class.html

### Wifi

`m5core2_wifi` has a WiFi example that includes IPv6.

You need to set your wifi name and password as environment variables to run:

```
export PIO_WIFI_SSID=YourWifiName
export PIO_WIFI_SSID=YourWifiPassword
pio run --target upload
```

Based on:

* Arduino-ESP32 IPv6 example: https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/examples/WiFiIPv6/WiFiIPv6.ino
* M5 Core 2 Advanced Wifi example: https://github.com/m5stack/M5Core2/tree/master/examples
* Setting DNS: https://www.keithh.com/network/ipv6-setup-esp8266/

TODO:

* Look at provisioning: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/index.html

### MQTT

You need to set your wifi and MQTT credentials to run:

```shell
export PIO_WIFI_SSID=YourWifiName
export PIO_WIFI_PASSWORD=YourWifiPassword
export PIO_MQTT_USER=mqttuser
export PIO_MQTT_PASSWORD=YourMQTTPassword
pio run --target upload
```

You can see connection attempts by logging into the Mosquitto server and following the log:

```
ssh iotadmin@mqtt-0xacc5-dev.australiaeast.cloudapp.azure.com
...
sudo tail -f /var/log/mosquitto/mosquitto.log
```

You can also run a client to receive (or send messages):

```
export MQTT_PASSWORD=YourSecretPassword
mosquitto_sub -h mqtt-0xacc5-dev-ipv4.australiaeast.cloudapp.azure.com -t test -p 8883 -u mqttuser -P $MQTT_PASSWORD
...
mosquitto_pub -h mqtt-0xacc5-dev.australiaeast.cloudapp.azure.com -t test -m 'Hello Command' -p 8883 -u mqttuser -P $MQTT_PASSWORD
```

* PubSubClient, https://github.com/knolleary/pubsubclient
* Bundle PubSubClient & WiFi, https://github.com/plapointe6/EspMQTTClient
* Alernative MQTT https://github.com/256dpi/arduino-mqtt


M5Stack Core2 to Azure (via WiFi)
---------------------------------

We need to add both the M5Core2 library and the Azure SDK, e.g. if you were creating from scratch:

```shell
mkdir m5core2_azureiot
cd m5core2_azureiot
pio project init --board m5stack-core2
pio pkg install --library M5Core2
pio pkg install --library "azure/Azure SDK for C"
```

To build (`pio run`) I had to make sure the lib_deps section had just "azure/Azure SDK for C" (without any version).

Note that the Arduino (used by PlatformIO) port of the library is version 1.0.0-beta.5, which corresponds to the library version 1.3.2. PlatformIO registry information: https://registry.platformio.org/libraries/azure/Azure%20SDK%20for%20C

Arduino port source code (with samples; this is based on the ESP32 sample): https://github.com/Azure/azure-sdk-for-c-arduino

Note that the example is an Arduino IDE sketch file (.ino), although it is easy to convert to a normal C++ file to use with Platform IO. Note that it uses the lower level Espressif libraries for MQTT, rather than PubSubClient.

You need to set some Azure values. If Device ID is blank, the app will use the device EUI-64 (based on the MAC), and print out the details (on screen and to the serial logging) so you can register it in IoT Hub and get the device key.

In PlatformIO CLI, monitor the serial port to see the Device ID:

```shell
pio device monitor --baud 115200
```

To register the device and output the host name and device key:

```powershell
az login
$deviceId = "eui-0a3af2fffe65db28"
$iotName = "iot-hub001-0x$((az account show --query id --output tsv).Substring(0,4))-dev"
$iotHub = az iot hub show --name $iotName | ConvertFrom-Json
$device = az iot hub device-identity create -n $iotName -d $deviceId | ConvertFrom-Json
$iotHub.properties.hostName
$device.authentication.symmetricKey.primaryKey
```

Once you have the host and device key, you can set the values (along with your wifi credentials) and run the application.

```shell
export PIO_WIFI_SSID=YourWifiName
export PIO_WIFI_PASSWORD=YourWifiPassword
export IOT_CONFIG_DEVICE_ID=
export IOT_CONFIG_IOTHUB_FQDN=YourAzureIotHostName.azure-devices.net
export IOT_CONFIG_DEVICE_KEY=YourAzureIoTDeviceKey
pio run --target upload
```

While running, to monitor the IoT Hub events:

```powershell
az login
$deviceId = "eui-0a3af2fffe65db28"
$iotName = "iot-hub001-0x$((az account show --query id --output tsv).Substring(0,4))-dev"
$connection = az iot hub connection-string show --hub-name $iotName | ConvertFrom-Json
az iot hub monitor-events --login $connection.connectionString --device-id $deviceId
```

Device ID notes:

* Device ID: max 128 characters, alphanumeric plus -.+%_#*?!(),:=@'
* Registration ID (DPS): max 128 characters, alphanumeric plus -._:
* RFC9039 Uniform Resource Names for Device Identifiers, start with "urn:dev:", with alphanumeric plus -._: at various locations, e.g. urn:dev:mac:0224befffe804ff1
* RFC7254 The GSM and IEI URN, e.g. "urn:gsma:imei:90420156-025763-0;vers=0"


Library references
------------------

PlatformIO library registry: https://registry.platformio.org/

* M5Core2 library: https://registry.platformio.org/libraries/m5stack/M5Core2
* AWSLabs SimpleIoT: https://github.com/awslabs/simpleiot-arduino


https://github.com/khoih-prog/ESP_WiFiManager


* Video on ESP32 IPv6, https://www.youtube.com/watch?v=mn0imqzramQ
  - CONFIG_IPV6_DHCP6_OTHER_FLAGS => "-DLWIP_IPV6_DHCP6=1"
  - CONFIG_IPV6_RA_RDNSS => "-DLWIP_ND6_RDNSS_MAX_DNS_SERVERS=1"
  - CONFIG_IPV6_GLOBAL_AUTOCONFIG => ?
  -  => ESP_IPV6_AUTOCONFIG
  - See: https://github.com/espressif/arduino-esp32/blob/master/tools/sdk/esp32c3/include/lwip/lwip/src/include/lwip/opt.h

* Programming guide: https://docs.espressif.com/projects/esp-idf/en/v4.2.2/esp32/api-reference/network/esp_netif.html

TODO
----

core2_azure_simfactory

core2_nbiot_simfactory

core2_azure_factory


