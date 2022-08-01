#include "EventLogger.h"

#include <M5Atom.h>

#define ST(A) #A
#define STR(A) ST(A)

bool led = true;
EventLogger *logger = new EventLogger();
char *version = STR(PIO_VERSION);

void setup() {
  M5.begin(true, true, true);
  delay(10);
  logger->begin();
  logger->information("Atom started, v%s", version);
  M5.dis.fillpix(CRGB::Green); 
}

void loop() {
  M5.update();
  logger->loop();
  if (M5.Btn.wasPressed()) {
    led = !led;
    logger->information("Button was pressed %s\n", led ? "on" : "off");
    if (led) {
      M5.dis.fillpix(CRGB::Blue);
    } else {
      M5.dis.clear();
    }
  }
}
