#include "AtomLogger.h"

#include <M5Atom.h>

AtomLogger::AtomLogger() {}

static const char *TAG = "AtomLogger";

// Protected

void AtomLogger::log(esp_log_level_t level, const char *message) {
  if (level == ESP_LOG_ERROR) {
    M5.dis.fillpix(CRGB::Red);
    ESP_LOGE(TAG, "ATOM: %s", message);
  } else {
    M5.dis.fillpix(CRGB::Blue);
    ESP_LOGI(TAG, "ATOM: %s", message);
  }
}
