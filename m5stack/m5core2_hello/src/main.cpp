#include <Arduino.h>
#include <M5Core2.h>

void setup() {
  M5.begin();
  M5.Lcd.setTextSize(3);
  M5.Lcd.print("Hello M5Stack Core2\n(with Arduino framework)\nvia PlatformIO");
  M5.Lcd.print("AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz\n");
  for (int x = 1; x < 100; x++) {
    M5.Lcd.printf("%d", x % 10);
  }
  M5.Lcd.println();
  for (int y = 1; y < 25; y++) {
    M5.Lcd.printf("%d\n", y);
  }
}

void loop() {
}
