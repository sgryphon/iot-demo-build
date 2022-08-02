#include "AtomLogger.h"
#include "WiFiNetworkManager.h"

#include <WiFi.h>
#include <M5Atom.h>

#define ST(A) #A
#define STR(A) ST(A)

const char *ap_password = STR(PIO_AP_PASSWORD);
int16_t count = 0;
//EventLogger *logger = new EventLogger();
EventLogger *logger = new AtomLogger();
NetworkManager *network = nullptr;
const char *version = STR(PIO_VERSION);
const char *wifi_password = STR(PIO_WIFI_PASSWORD);
const char *wifi_ssid = STR(PIO_WIFI_SSID);

void setup() {
  M5.begin(true, true, true);
  delay(10);
  logger->begin();
  logger->information("Atom started, v%s", version);

  WiFiNetworkManager *wiFiNetwork = new WiFiNetworkManager();
  wiFiNetwork->setEventLogger(logger);
  wiFiNetwork->setCredentials(ap_password, wifi_ssid, wifi_password);
  network = wiFiNetwork;
  network->begin();
}

void loop() {  
  M5.update();
  logger->loop();
  network->loop();
  if (M5.Btn.wasPressed()) {
    ++count;
    if (count % 5 == 0) {
      logger->error("Too many button presses");
      return;
    }
    logger->information("Button was pressed %d\n", count);
  }
}
