#include "EventLogger.h"
#include "WiFiNetworkManager.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <M5Unified.h>

#define ST(A) #A
#define STR(A) ST(A)

const char *ap_password = STR(PIO_AP_PASSWORD);
int16_t count = 0;
EventLogger *logger = new EventLogger();
//EventLogger *logger = new AtomLogger();
//EventLogger *logger = new Core2Logger();
WiFiNetworkManager *network = nullptr;
extern const uint8_t root_ca_pem_start[] asm("_binary_src_certs_USERTrust_RSA_Certification_Authority_pem_start");
extern const uint8_t root_ca_pem_end[] asm("_binary_src_certs_USERTrust_RSA_Certification_Authority_pem_end");
const char *version = STR(PIO_VERSION);
const char *wifi_password = STR(PIO_WIFI_PASSWORD);
const char *wifi_ssid = STR(PIO_WIFI_SSID);

void testNetwork() {
  int scenario = (count - 1) % 4;
  logger->information("Button %d, scenario %d, v%s", count, scenario, version);

  // Naming here is wonky:
  // localIP() is the local IPv4 address (as opposed to remote IPv4 address)
  // globalIPv6() is the first local IPv6 address of category global
  // localIPv6() is the local IPv6 address of category link-local
  logger->information("Network: Global IPv6 %s, IPv4 %s, Link-Local IPv6 %s",
    WiFi.globalIPv6().toString().c_str(), 
    WiFi.localIP().toString().c_str(),
    WiFi.localIPv6().toString().c_str()
  );
  logger->information("DNS %s, DNS2 %s, DNS3 %s",
    WiFi.dnsIP(0).toString().c_str(),
    WiFi.dnsIP(1).toString().c_str(),
    WiFi.dnsIP(2).toString().c_str()
  );

  if (scenario < 3) {
    String url;
    if (scenario == 0) {
      url = "http://v4v6.ipv6-test.com/api/myip.php";
    } else if (scenario == 1) {
      url = "http://v6.ipv6-test.com/api/myip.php";
    } else {
      url = "http://v4.ipv6-test.com/api/myip.php";
    }

    logger->information("URL: %s", url.c_str());

    HTTPClient http;
    bool success = http.begin(url);
    if (!success) {
      logger->error("Unable to begin HTTP");
      return;
    }

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK && httpCode != HTTP_CODE_MOVED_PERMANENTLY) {
      logger->error("HTTP GET error %d: %s", httpCode, http.errorToString(httpCode).c_str());
      return;
    }

    String payload = http.getString();
    logger->information("response=<%s>", payload.c_str());
    if (payload.indexOf(":") >= 0) {
      logger->success();
    } else {
      logger->warning();
    }

  } else {
    String url = "https://v4v6.ipv6-test.com/api/myip.php";
    logger->information("TLS URL: %s", url.c_str());

    WiFiClientSecure *client = new WiFiClientSecure;
    if (!client) {
      logger->error("Unable to create secure client");
      return;
    }
    client->setCACert((char *)root_ca_pem_start);

    HTTPClient http;
    bool success = http.begin(*client, url);
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
    logger->information("response=<%s>", payload.c_str());
    if (payload.indexOf(":") >= 0) {
      logger->success();
    } else {
      logger->warning();
    }
    delete client;
  }
}

void setup() {
  M5.begin();
  delay(10);
  logger->begin();
  logger->information("M5 started, v%s", version);

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
  if (M5.BtnA.wasPressed()) {
    ++count;
    if (count % 5 == 0) {
      logger->error("Too many button presses");
      return;
    }
    testNetwork();
    //logger->information("Button was pressed %d\n", count);
  }
}
