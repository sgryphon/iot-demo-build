M5 Stack
========

Website: https://m5stack.com/

Core / Fire / Core2 / Core2 AWS: Stackable modules with screen and buttons, and lots of options to expand and add sensors. (5cm x 5cm)

Atom: Miniature devices that are just the controller, with a range of add-on sensors available. (2.5cm x 2.5cm)

Stick: Full feature miniature devices.


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


Azure (via WiFi)
----------------

core2_azure_simfactory

core2_nbiot_simfactory

core2_azure_factory


