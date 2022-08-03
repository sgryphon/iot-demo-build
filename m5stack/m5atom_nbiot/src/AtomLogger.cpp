#include "AtomLogger.h"

#include <M5Atom.h>

AtomLogger::AtomLogger() {}

static const char *TAG = "AtomLogger";

CRGB led_color = CRGB::White;
int16_t led_count_remaining = 0;
int16_t led_interval_ms = 500;
unsigned long led_on_at_millis = 0;
unsigned long led_off_at_millis = 0;
int16_t led_time_on_ms = 200;

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

void startLed(CRGB color, int16_t count, int16_t time_on_ms = 200) {
  led_color = color;
  led_time_on_ms = time_on_ms;
  led_count_remaining = count > 0 ? count - 1 : count;
  ledOn(millis());
}

// Public

void AtomLogger::loop() {
  unsigned long now = millis();
  if (led_off_at_millis > 0 && now > led_off_at_millis) {
    led_off_at_millis = 0;
    ESP_LOGD(TAG, "LED off");
    M5.dis.clear();
  }
  if (led_on_at_millis > 0 && now > led_on_at_millis) {
    ledOn(now);
  }
}

void AtomLogger::pending() { 
  startLed(CRGB::Yellow, -1);
  ESP_LOGI(TAG, "Pending");
}

void AtomLogger::success() { 
  startLed(CRGB::Green, 2, 500);
  ESP_LOGI(TAG, "Success");
}

void AtomLogger::warning() { 
  startLed(CRGB::Orange, 1, 400);
  ESP_LOGW(TAG, "Warning");
}

// Protected

void AtomLogger::log(esp_log_level_t level, const char *message) {
  if (level == ESP_LOG_ERROR) {
    startLed(CRGB::Red, 3);
    ESP_LOGE(TAG, "ATOM: %s", message);
  } else {
    startLed(CRGB::Blue, 1);
    ESP_LOGI(TAG, "ATOM: %s", message);
  }
}
