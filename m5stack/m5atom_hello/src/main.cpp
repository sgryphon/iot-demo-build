#include <M5Unified.h>
#include <FastLED.h>
#include <Arduino.h>

static const char *TAG = "Hello";

#define NUM_LEDS 1
#define DATA_PIN 27

bool led = true;
CRGB leds[NUM_LEDS];

void setup() {
  M5.begin();
  delay(10);
  Serial.println("Atom started");
  FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
  leds[0] = CRGB::Green;
  FastLED.show();
}

void loop() {
  M5.update();
  if (M5.BtnA.wasPressed()) {
    ESP_LOGD(TAG, "Button pressed");
    led = !led;
    Serial.printf("Button was pressed %s\n", led ? "on" : "off");
    if (led) {
      leds[0] = CRGB::Blue;
    } else {
      leds[0] = CRGB::Black;
    }
    FastLED.show();
  }
}
