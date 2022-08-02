#include "AtomLogger.h"

#include <M5Atom.h>

#define ST(A) #A
#define STR(A) ST(A)

int16_t count = 0;
//EventLogger *logger = new EventLogger();
EventLogger *logger = new AtomLogger();
const char *version = STR(PIO_VERSION);

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
    ++count;
    if (count % 5 == 0) {
      logger->error("Too many button presses");
      return;
    }
    logger->information("Button was pressed %d\n", count);
  }
}
