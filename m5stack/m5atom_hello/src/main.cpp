#include <M5Atom.h>
#include <Arduino.h>

static const char *TAG = "Hello";

bool led = true;

void setup() {
  M5.begin(true, true, true);
  delay(10);
  Serial.println("Atom started");
  M5.dis.fillpix(CRGB::Green); 
}

void loop() {
  M5.update();
  if (M5.Btn.wasPressed()) {
    ESP_LOGD(TAG, "Button pressed");
    led = !led;
    Serial.printf("Button was pressed %s\n", led ? "on" : "off");
    if (led) {
      M5.dis.fillpix(CRGB::Blue);
    } else {
      M5.dis.clear();
    }
  }
}
