// M5Stack Core S3 device configuration for ESP Matter
//
// This configures the hardware abstraction for M5Stack Core S3
// including button and basic GPIO for compatibility with ESP Matter

#include <esp_log.h>
#include <iot_button.h>
#include <button_gpio.h>
#include <driver/gpio.h>
#include <led_driver.h>

/* M5Stack Core S3 pin definitions */
#define BUTTON_GPIO_PIN        0   /* Boot button on Core S3 */
#define LED_GPIO_PIN           46  /* Backlight pin - we'll use this as a fallback LED */
#define LED_CHANNEL            7   /* LEDC channel for LED control */

led_driver_config_t led_driver_get_config()
{
    led_driver_config_t config = {
        .gpio = LED_GPIO_PIN,
        .channel = LED_CHANNEL,
    };
    return config;
}

button_gpio_config_t button_driver_get_config()
{
    button_gpio_config_t config = {
        .gpio_num = BUTTON_GPIO_PIN,
        .active_level = 0,  /* Active low */
    };
    return config;
}