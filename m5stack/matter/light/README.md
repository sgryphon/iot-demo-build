# M5Stack Core S3 Matter over Thread Light Demo

This project demonstrates a Matter over Thread light device running on the M5Stack Core S3 with the H2 Gateway module.

## Features

- **Matter Device Type**: On/Off Light (0x100)
- **Network**: Thread using M5Stack H2 Gateway module
- **Commissioning**: Bluetooth LE for initial setup
- **Display**: Shows light state on Core S3 screen
  - Light grey rectangle when OFF
  - Blue rectangle when ON
- **Compatible with**: Google Home, Home Assistant, and other Matter controllers

## Hardware Requirements

- M5Stack Core S3
- M5Stack H2 Gateway module (for Thread networking)
- Thread Border Router (for network connectivity)

## Software Requirements

- ESP-IDF v5.1 or later
- ESP Matter library
- CMake and build tools

## Setup Instructions

### 1. Install ESP-IDF

```bash
# Install ESP-IDF v5.1 or later
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
git checkout v5.1
./install.sh
source export.sh
```

### 2. Install ESP Matter

```bash
# Clone ESP Matter repository
git clone --recursive https://github.com/espressif/esp-matter.git
cd esp-matter
git checkout main
```

### 3. Set Environment Variables

```bash
export ESP_MATTER_PATH=/path/to/esp-matter
export IDF_PATH=/path/to/esp-idf
```

### 4. Build and Flash

```bash
# Set target chip
idf.py set-target esp32s3

# Configure project (optional)
idf.py menuconfig

# Build project
idf.py build

# Flash to device
idf.py -p /dev/ttyUSB0 flash monitor
```

## Configuration

### Thread Network

The device uses the M5Stack H2 Gateway module for Thread networking. Make sure:

1. The H2 Gateway module is properly connected to the Core S3
2. A Thread Border Router is available on your network
3. The Thread channel is correctly configured (default: channel 15)

### Matter Commissioning

1. **Open Commissioning Window**: The device opens a commissioning window on startup
2. **Use Matter Controller**: 
   - Google Home: "Add device" → "Matter device"
   - Home Assistant: Settings → Integrations → Add Integration → Matter
3. **Follow Setup**: Scan QR code or enter manual pairing code
4. **Network Assignment**: Device will join your Thread network via the border router

### Display Configuration

The display shows a centered rectangle:
- **Position**: Center of 320x240 screen
- **Size**: 100x80 pixels
- **Colors**:
  - Light grey (0x8410) when OFF
  - Blue (0x001F) when ON

## Project Structure

```
m5stack/matter/light/
├── CMakeLists.txt              # Main build configuration
├── partitions.csv              # Flash partition table
├── sdkconfig.defaults          # Default configuration
├── sdkconfig.defaults.esp32s3  # ESP32-S3 specific config
├── main/
│   ├── CMakeLists.txt          # Main component build
│   ├── app_main.cpp            # Application entry point
│   ├── app_driver.cpp          # Matter attribute handlers
│   ├── app_priv.h              # Private header
│   └── display_driver.cpp      # M5Stack display control
└── device_hal/
    └── m5stack_cores3/
        ├── device.c            # Hardware abstraction
        └── esp_matter_device.cmake
```

## Pin Configuration

| Function | GPIO | Description |
|----------|------|-------------|
| Boot Button | 0 | Commissioning trigger |
| Backlight | 46 | Display backlight control |

## Usage

1. **Power On**: Device starts and opens commissioning window
2. **Commission**: Use Matter controller to commission device
3. **Control**: Send on/off commands through Matter network
4. **Visual Feedback**: Rectangle on screen changes color based on state

## Troubleshooting

### Build Issues

- Ensure ESP-IDF and ESP Matter paths are correctly set
- Check that all submodules are properly initialized
- Verify ESP32-S3 target is selected

### Commissioning Issues

- Check that Bluetooth is enabled on your controller device
- Ensure device is in commissioning mode (check logs)
- Verify Thread Border Router is operational

### Network Issues

- Check Thread network topology
- Verify H2 Gateway module connection
- Monitor Thread commissioning process in logs

## Development

### Adding Features

- Modify `app_driver.cpp` for new attribute handlers
- Update `display_driver.cpp` for enhanced visual feedback
- Add new clusters/endpoints in `app_main.cpp`

### Debugging

```bash
# Monitor serial output
idf.py monitor

# Enable debug logging
idf.py menuconfig → Component config → Log output → Default log verbosity → Debug
```

## References

- [ESP Matter Documentation](https://docs.espressif.com/projects/esp-matter/en/latest/)
- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [Matter Specification](https://csa-iot.org/all-solutions/matter/)
- [M5Stack Core S3 Documentation](https://docs.m5stack.com/en/core/CoreS3)

## License

This example is provided under the Apache 2.0 License.