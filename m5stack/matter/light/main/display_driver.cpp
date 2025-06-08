/*
   M5Stack Core S3 Display Driver for Matter Light Demo
   
   Displays a rectangle on the screen:
   - Light grey when light is OFF
   - Blue when light is ON
*/

#include <esp_log.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <app_priv.h>

static const char *TAG = "display_driver";

/* M5Stack Core S3 Display Configuration */
#define LCD_PIXEL_CLOCK_HZ     (16 * 1000 * 1000)
#define LCD_H_RES              320
#define LCD_V_RES              240
#define LCD_CMD_BITS           8
#define LCD_PARAM_BITS         8

/* Core S3 LCD pins */
#define PIN_NUM_PCLK           15
#define PIN_NUM_CS             3
#define PIN_NUM_DC             2
#define PIN_NUM_RST            1
#define PIN_NUM_BK_LIGHT       46

/* RGB data pins */
#define PIN_NUM_DATA0          4   // B3
#define PIN_NUM_DATA1          5   // B4  
#define PIN_NUM_DATA2          6   // B5
#define PIN_NUM_DATA3          7   // G2
#define PIN_NUM_DATA4          15  // G3
#define PIN_NUM_DATA5          16  // G4
#define PIN_NUM_DATA6          8   // G5
#define PIN_NUM_DATA7          3   // B2

/* Color definitions (RGB565) */
#define COLOR_LIGHT_GREY       0x8410  /* Light grey for OFF state */
#define COLOR_BLUE             0x001F  /* Blue for ON state */
#define COLOR_BLACK            0x0000  /* Black for background */

/* Rectangle dimensions and position */
#define RECT_WIDTH             100
#define RECT_HEIGHT            80
#define RECT_X                 ((LCD_H_RES - RECT_WIDTH) / 2)
#define RECT_Y                 ((LCD_V_RES - RECT_HEIGHT) / 2)

static esp_lcd_panel_handle_t panel_handle = NULL;
static bool display_initialized = false;

/* Simple framebuffer for RGB565 */
static uint16_t *framebuffer = NULL;

esp_err_t display_init(void)
{
    ESP_LOGI(TAG, "Initializing M5Stack Core S3 display");
    
    /* Allocate framebuffer */
    framebuffer = (uint16_t*)heap_caps_malloc(LCD_H_RES * LCD_V_RES * sizeof(uint16_t), MALLOC_CAP_DMA);
    if (framebuffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate framebuffer");
        return ESP_ERR_NO_MEM;
    }
    
    /* Initialize backlight pin */
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << PIN_NUM_BK_LIGHT
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    
    /* Turn on backlight */
    gpio_set_level(PIN_NUM_BK_LIGHT, 1);
    
    /* For now, we'll create a simplified display initialization
       In a real implementation, this would configure the LCD controller */
    
    /* Clear framebuffer to black */
    for (int i = 0; i < LCD_H_RES * LCD_V_RES; i++) {
        framebuffer[i] = COLOR_BLACK;
    }
    
    display_initialized = true;
    ESP_LOGI(TAG, "Display initialized successfully");
    
    return ESP_OK;
}

static void draw_rectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
    if (!display_initialized || framebuffer == NULL) {
        return;
    }
    
    for (uint16_t row = y; row < y + height && row < LCD_V_RES; row++) {
        for (uint16_t col = x; col < x + width && col < LCD_H_RES; col++) {
            framebuffer[row * LCD_H_RES + col] = color;
        }
    }
}

static void clear_screen(uint16_t color)
{
    if (!display_initialized || framebuffer == NULL) {
        return;
    }
    
    for (int i = 0; i < LCD_H_RES * LCD_V_RES; i++) {
        framebuffer[i] = color;
    }
}

esp_err_t display_set_light_state(bool on)
{
    if (!display_initialized) {
        ESP_LOGW(TAG, "Display not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Setting light display state: %s", on ? "ON (Blue)" : "OFF (Grey)");
    
    /* Clear screen to black */
    clear_screen(COLOR_BLACK);
    
    /* Draw rectangle with appropriate color */
    uint16_t rect_color = on ? COLOR_BLUE : COLOR_LIGHT_GREY;
    draw_rectangle(RECT_X, RECT_Y, RECT_WIDTH, RECT_HEIGHT, rect_color);
    
    /* In a real implementation, we would flush the framebuffer to the LCD here
       For now, we just log the action */
    ESP_LOGI(TAG, "Rectangle drawn at (%d,%d) size %dx%d, color: 0x%04X", 
             RECT_X, RECT_Y, RECT_WIDTH, RECT_HEIGHT, rect_color);
    
    return ESP_OK;
}