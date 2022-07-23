#include <Arduino.h>
#include <M5Core2.h>

void setup() {
  M5.begin();
  M5.Lcd.print("Hello M5Stack Core2\n(with Arduino framework)\nvia PlatformIO");
}

void loop() {
}
