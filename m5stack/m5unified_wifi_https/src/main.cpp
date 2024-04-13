#include "Core2Logger.h"
#include "WiFiNetworkService.h"

#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <WiFi.h>
#include <M5Unified.h>

#define ST(A) #A
#define STR(A) ST(A)

const char *ap_password = STR(PIO_AP_PASSWORD);
int16_t count = 0;
//EventLogger *logger = new EventLogger();
//EventLogger *logger = new AtomLogger();
EventLogger *logger = new Core2Logger();
WiFiNetworkService *network = nullptr;
extern const uint8_t root_ca_pem_start[] asm("_binary_src_certs_USERTrust_RSA_Certification_Authority_pem_start");
extern const uint8_t root_ca_pem_end[] asm("_binary_src_certs_USERTrust_RSA_Certification_Authority_pem_end");
const char *version = STR(PIO_VERSION);
const char *wifi_password = STR(PIO_WIFI_PASSWORD);
const char *wifi_ssid = STR(PIO_WIFI_SSID);

const String destinations[] = {
  "http://v4v6.ipv6-test.com/api/myip.php",
  "http://v6.ipv6-test.com/api/myip.php",
  "http://v4.ipv6-test.com/api/myip.php",
  "https://v4v6.ipv6-test.com/api/myip.php",
  "https://v6.ipv6-test.com/api/myip.php",
  "https://v4.ipv6-test.com/api/myip.php",
};
const int destinations_length = sizeof(destinations) / sizeof(destinations[0]);

void testNetwork() {
  int scenario = (count - 1) % (destinations_length + 1);
  logger->information("Button %d, scenario %d, v%s", count, scenario, version);
  logger->information("Global IPv6 %s", WiFi.globalIPv6().toString().c_str());
  logger->information("IPv4 %s", WiFi.localIP().toString().c_str());
  WiFi.STA.printTo(Serial);

  for (int dns_index = 0; dns_index < 2; ++dns_index) {
    logger->information("DNS%d %s", dns_index, WiFi.dnsIP(dns_index).toString().c_str());
  }

  String url = destinations[scenario];

  if (url.startsWith("http://")) {
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
    if (payload.indexOf(":") >= 0 || payload.indexOf(".") >= 0) {
      logger->success();
    } else {
      logger->warning();
    }

  } else { // HTTPS
    logger->information("TLS URL: %s", url.c_str());

    NetworkClientSecure *client = new NetworkClientSecure;
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
    if (payload.indexOf(":") >= 0 || payload.indexOf(".") >= 0) {
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

  WiFiNetworkService *wiFiNetwork = new WiFiNetworkService();
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
    int scenario = (count - 1) % (destinations_length + 1);
    if (scenario == destinations_length) {
      logger->error("Test error - max scenarios reached");
      return;
    }
    testNetwork();
    //logger->information("Button was pressed %d\n", count);
  }
}
