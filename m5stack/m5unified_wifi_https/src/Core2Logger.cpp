#include "Core2Logger.h"

#include <M5Unified.h>

Core2Logger::Core2Logger() {}

static const char *TAG = "Core2Logger";

/*
CRGB led_color = CRGB::White;
int16_t led_count_remaining = 0;
int16_t led_interval_ms = 500;
unsigned long led_on_at_millis = 0;
unsigned long led_off_at_millis = 0;
int16_t led_time_on_ms = 200;
*/

/*
void ledOn(unsigned long now) {
  led_off_at_millis = now + led_time_on_ms;
  // Next on
  if (led_count_remaining == 0) {
    led_on_at_millis = 0;
  } else {
    led_on_at_millis = now + led_interval_ms;
    if (led_count_remaining > 0) {
      led_count_remaining--;
    }
  }
  ESP_LOGD(TAG, "LED on %08x (off %d)", led_color, led_off_at_millis);
  M5.dis.fillpix(led_color);
}
*/

/*
void startLed(CRGB color, int16_t count, int16_t time_on_ms = 200) {
  led_color = color;
  led_time_on_ms = time_on_ms;
  led_count_remaining = count > 0 ? count - 1 : count;
  ledOn(millis());
}
*/

// Public

void Core2Logger::loop() {
  unsigned long now = millis();
  /*
  if (led_off_at_millis > 0 && now > led_off_at_millis) {
    led_off_at_millis = 0;
    ESP_LOGD(TAG, "LED off");
    M5.dis.clear();
  }
  if (led_on_at_millis > 0 && now > led_on_at_millis) {
    ledOn(now);
  }
  */
}

void Core2Logger::pending() { 
  //startLed(CRGB::Yellow, -1);
  ESP_LOGI(TAG, "Pending");
}

void Core2Logger::success() { 
  //startLed(CRGB::Green, 2, 500);
  ESP_LOGI(TAG, "Success");
}

void Core2Logger::warning() { 
  //startLed(CRGB::Orange, 1, 400);
  ESP_LOGW(TAG, "Warning");
}

// Protected

void Core2Logger::log(esp_log_level_t level, const char *message) {
  if (level == ESP_LOG_ERROR) {
    //startLed(CRGB::Red, 3);
    ESP_LOGE(TAG, "CORE2: %s", message);
  } else {
    //startLed(CRGB::Blue, 1);
    ESP_LOGI(TAG, "CORE2: %s", message);
  }
}
