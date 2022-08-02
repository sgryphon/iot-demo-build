#include "AtomLogger.h"
#include "WiFiNetworkManager.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <M5Atom.h>

#define ST(A) #A
#define STR(A) ST(A)

const char *ap_password = STR(PIO_AP_PASSWORD);
int16_t count = 0;
//EventLogger *logger = new EventLogger();
EventLogger *logger = new AtomLogger();
WiFiNetworkManager *network = nullptr;
extern const uint8_t root_ca_pem_start[] asm("_binary_src_certs_USERTrust_RSA_Certification_Authority_pem_start");
extern const uint8_t root_ca_pem_end[] asm("_binary_src_certs_USERTrust_RSA_Certification_Authority_pem_end");
const char *version = STR(PIO_VERSION);
const char *wifi_password = STR(PIO_WIFI_PASSWORD);
const char *wifi_ssid = STR(PIO_WIFI_SSID);

void testNetwork() {
  logger->information("Button %d, IPv6 %s, IPv4 %s", count, network->globalIPv6().toString().c_str(), WiFi.localIP().toString().c_str());

  WiFiClientSecure *client = new WiFiClientSecure;
  if (!client) {
    logger->error("Unable to create secure client");
    return;
  }
  client->setCACert((char *)root_ca_pem_start);

  HTTPClient http;
  bool success = http.begin(*client, "https://v4v6.ipv6-test.com/api/myip.php");
  if (!success) {
    logger->error("Unable to begin HTTP");
    delete client;
    return;
  }

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK && httpCode != HTTP_CODE_MOVED_PERMANENTLY) {
    logger->error("HTTP GET error %d: %s", httpCode, http.errorToString(httpCode).c_str());
    delete client;
    return;
  }

  String payload = http.getString();
  logger->information("ipv6-test: %s", payload.c_str());
  delete client;
}

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
    testNetwork();
    //logger->information("Button was pressed %d\n", count);
  }
}
