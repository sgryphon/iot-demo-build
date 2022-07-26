#include <Arduino.h>
#include <M5Core2.h>
#include <WiFi.h>
//#include <WiFiMulti.h>
#include <HTTPClient.h>

#include "StartNetwork.h"
#include "wifi_config.h"

static const char* ssid = WIFI_SSID;
static const char* password = WIFI_PASSWORD;

#define SEND_INTERVAL_MS (10000)
unsigned long nextMessageMilliseconds = 0;
HTTPClient http;

#define HEADER_HEIGHT (16)

// https://docs.platformio.org/en/latest/platforms/espressif32.html#embedding-binary-data
// https://techtutorialsx.com/2017/11/18/esp32-arduino-https-get-request/

extern const uint8_t root_ca_pem_start[] asm("_binary_src_certs_USERTrust_RSA_Certification_Authority_pem_start");
extern const uint8_t root_ca_pem_end[] asm("_binary_src_certs_USERTrust_RSA_Certification_Authority_pem_end");

const char* root_ca= \
"-----BEGIN CERTIFICATE-----\n" \
"MIIF3jCCA8agAwIBAgIQAf1tMPyjylGoG7xkDjUDLTANBgkqhkiG9w0BAQwFADCB\n" \
"iDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0pl\n" \
"cnNleSBDaXR5MR4wHAYDVQQKExVUaGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNV\n" \
"BAMTJVVTRVJUcnVzdCBSU0EgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMTAw\n" \
"MjAxMDAwMDAwWhcNMzgwMTE4MjM1OTU5WjCBiDELMAkGA1UEBhMCVVMxEzARBgNV\n" \
"BAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0plcnNleSBDaXR5MR4wHAYDVQQKExVU\n" \
"aGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNVBAMTJVVTRVJUcnVzdCBSU0EgQ2Vy\n" \
"dGlmaWNhdGlvbiBBdXRob3JpdHkwggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIK\n" \
"AoICAQCAEmUXNg7D2wiz0KxXDXbtzSfTTK1Qg2HiqiBNCS1kCdzOiZ/MPans9s/B\n" \
"3PHTsdZ7NygRK0faOca8Ohm0X6a9fZ2jY0K2dvKpOyuR+OJv0OwWIJAJPuLodMkY\n" \
"tJHUYmTbf6MG8YgYapAiPLz+E/CHFHv25B+O1ORRxhFnRghRy4YUVD+8M/5+bJz/\n" \
"Fp0YvVGONaanZshyZ9shZrHUm3gDwFA66Mzw3LyeTP6vBZY1H1dat//O+T23LLb2\n" \
"VN3I5xI6Ta5MirdcmrS3ID3KfyI0rn47aGYBROcBTkZTmzNg95S+UzeQc0PzMsNT\n" \
"79uq/nROacdrjGCT3sTHDN/hMq7MkztReJVni+49Vv4M0GkPGw/zJSZrM233bkf6\n" \
"c0Plfg6lZrEpfDKEY1WJxA3Bk1QwGROs0303p+tdOmw1XNtB1xLaqUkL39iAigmT\n" \
"Yo61Zs8liM2EuLE/pDkP2QKe6xJMlXzzawWpXhaDzLhn4ugTncxbgtNMs+1b/97l\n" \
"c6wjOy0AvzVVdAlJ2ElYGn+SNuZRkg7zJn0cTRe8yexDJtC/QV9AqURE9JnnV4ee\n" \
"UB9XVKg+/XRjL7FQZQnmWEIuQxpMtPAlR1n6BB6T1CZGSlCBst6+eLf8ZxXhyVeE\n" \
"Hg9j1uliutZfVS7qXMYoCAQlObgOK6nyTJccBz8NUvXt7y+CDwIDAQABo0IwQDAd\n" \
"BgNVHQ4EFgQUU3m/WqorSs9UgOHYm8Cd8rIDZsswDgYDVR0PAQH/BAQDAgEGMA8G\n" \
"A1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQEMBQADggIBAFzUfA3P9wF9QZllDHPF\n" \
"Up/L+M+ZBn8b2kMVn54CVVeWFPFSPCeHlCjtHzoBN6J2/FNQwISbxmtOuowhT6KO\n" \
"VWKR82kV2LyI48SqC/3vqOlLVSoGIG1VeCkZ7l8wXEskEVX/JJpuXior7gtNn3/3\n" \
"ATiUFJVDBwn7YKnuHKsSjKCaXqeYalltiz8I+8jRRa8YFWSQEg9zKC7F4iRO/Fjs\n" \
"8PRF/iKz6y+O0tlFYQXBl2+odnKPi4w2r78NBc5xjeambx9spnFixdjQg3IM8WcR\n" \
"iQycE0xyNN+81XHfqnHd4blsjDwSXWXavVcStkNr/+XeTWYRUc+ZruwXtuhxkYze\n" \
"Sf7dNXGiFSeUHM9h4ya7b6NnJSFd5t0dCy5oGzuCr+yDZ4XUmFF0sbmZgIn/f3gZ\n" \
"XHlKYC6SQK5MNyosycdiyA5d9zZbyuAlJQG03RoHnHcAP9Dc1ew91Pq7P8yF1m9/\n" \
"qS3fuQL39ZeatTXaw2ewh0qpKJ4jjv9cJ2vhsE/zB+4ALtRZh8tSQZXq9EfX7mRB\n" \
"VXyNWQKV3WKdwrnuWih0hKWbt5DHDAff9Yk2dDLWKMGwsAvgnEzDHNb842m1R0aB\n" \
"L6KCq9NjRHDEjf8tM7qtj3u1cIiuPhnPQCjY/MiQu12ZIvVS5ljFH4gxQ+6IHdfG\n" \
"jjxDah2nGN59PRbxYvnKkKj9\n" \
"-----END CERTIFICATE-----\n";

void printHeader() {
  // Time = 8, IPv6 = 39
  // Date = 10, MAC = 17, WiFi 3, IPv4 = 15
  RTC_DateTypeDef rtcDateNow;
  RTC_TimeTypeDef rtcTimeNow;
  M5.Rtc.GetDate(&rtcDateNow);
  M5.Rtc.GetTime(&rtcTimeNow);
  int x = M5.Lcd.getCursorX();
  int y = M5.Lcd.getCursorY();
  M5.Lcd.fillRect(0, 0, 320, HEADER_HEIGHT, M5.Lcd.color565(0x83, 0x4c, 0xc2));
  M5.Lcd.setTextColor(WHITE, M5.Lcd.color565(0x83, 0x4c, 0xc2));
  // Time
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.printf("%02d:%02d:%02d", rtcTimeNow.Hours, rtcTimeNow.Minutes, rtcTimeNow.Seconds);
  // Date
  M5.Lcd.setCursor(0, 8);
  M5.Lcd.printf("%04d-%02d-%02d", rtcDateNow.Year, rtcDateNow.Month, rtcDateNow.Date);
  // IPv6
  String ipv6 = StartNetwork.globalIPv6().toString();
  M5.Lcd.setCursor(53 * 6 - M5.Lcd.textWidth(ipv6), 0); 
  //M5.Lcd.setCursor((53-39)*6, 0);
  M5.Lcd.print(ipv6);
  // MAC & WiFi Status
  M5.Lcd.setCursor((53-39)*6, 8);
  M5.Lcd.printf("%s (%3d)", WiFi.macAddress().c_str(), WiFi.status());
  // IPv4
  String ipv4 = WiFi.localIP().toString();
  M5.Lcd.setCursor(53 * 6 - M5.Lcd.textWidth(ipv4), 8); 
  M5.Lcd.print(ipv4.c_str());

  M5.Lcd.setCursor(x, y);
  M5.Lcd.setTextColor(WHITE, BLACK);
}

void printWiFi() {
  M5.Lcd.print("IPv6: ");
  M5.Lcd.print(WiFi.localIPv6());
  M5.Lcd.print("\n");
  M5.Lcd.print("IPv6: ");
  M5.Lcd.print(StartNetwork.globalIPv6());
  M5.Lcd.print("\n");
  M5.Lcd.print("DNS: ");
  M5.Lcd.print(StartNetwork.mainDnsIP());
  M5.Lcd.print("\n");
  M5.Lcd.print("\n");

  M5.Lcd.print("IPv4: ");
  M5.Lcd.print(WiFi.localIP());
  M5.Lcd.print("\n");
  M5.Lcd.print("DNS: ");
  M5.Lcd.print(WiFi.dnsIP(0));
  M5.Lcd.print("\n");
  M5.Lcd.print("Gateway: ");
  M5.Lcd.print(WiFi.gatewayIP());
  M5.Lcd.print("\n");
  M5.Lcd.print("\n");
}

void wifiConnectedLoop() {
  unsigned long nowMilliseconds = millis();
  if (nowMilliseconds > nextMessageMilliseconds) {
    nextMessageMilliseconds = nowMilliseconds + SEND_INTERVAL_MS;

    int httpCode;
/*
    M5.Lcd.println(nowMilliseconds);
    M5.Lcd.print("v6: begin");
    http.begin("http://v6.ipv6-test.com/api/myip.php");
    M5.Lcd.print(",GET");
    httpCode = http.GET();
    M5.Lcd.printf(",%d", httpCode);
    if (httpCode > 0) {  // httpCode will be negative on error.
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        M5.Lcd.print(",");
        M5.Lcd.print(payload);
      }
    } else {export PIO_WIFI_SSID=GryphonIOT
export PIO_WIFI_PASSWORD=sporting4p9q
export PIO_MQTT_USER=mqttuser
export PIO_MQTT_PASSWORD=Pass@word1

      M5.Lcd.printf(",ERROR %s", http.errorToString(httpCode).c_str());
    }
    M5.Lcd.print("\n");
    http.end();
*/

    M5.Lcd.print("v4v6: begin");
    http.begin("http://v4v6.ipv6-test.com/api/myip.php");
    M5.Lcd.print(",GET");
    httpCode = http.GET();
    M5.Lcd.printf(",%d", httpCode);
    if (httpCode > 0) {  // httpCode will be negative on error.
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        M5.Lcd.print(",");
        M5.Lcd.print(payload);
      }
    } else {
      M5.Lcd.printf(",ERROR %s", http.errorToString(httpCode).c_str());
    }
    M5.Lcd.print("\n");
    http.end();

    // https://github.com/espressif/arduino-esp32/blob/master/libraries/HTTPClient/examples/BasicHttpsClient/BasicHttpsClient.ino

    M5.Lcd.print("v4v6-tls: ");
    WiFiClientSecure *client = new WiFiClientSecure;
    if (client) {
      client->setCACert((char *)root_ca_pem_start);
      //client->setCACert(root_ca);
      {
        HTTPClient http;
        M5.Lcd.print("begin");
        if (http.begin(*client, "https://v4v6.ipv6-test.com/api/myip.php")) {
          M5.Lcd.print(",GET");
          httpCode = http.GET();
          M5.Lcd.printf(",%d", httpCode);
          if (httpCode > 0) {
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
              String payload = http.getString();
              M5.Lcd.print(",");
              M5.Lcd.print(payload);
            }
          } else {
            M5.Lcd.printf(",ERROR %s", http.errorToString(httpCode).c_str());
          }

          http.end();
        } else {
          M5.Lcd.print("Unable to connect");
        }
        M5.Lcd.print("\n");
      }
      delete client;
    } else {
          M5.Lcd.print("Unable to create client\n");
    }
  } 
}

void setup() {
  M5.begin();

  printHeader();
  M5.Lcd.setCursor(0, HEADER_HEIGHT);
  printWiFi();

  M5.Lcd.printf("Connecting to %s", ssid);
  M5.Lcd.print("\n");

  if (ssid=="") {
    M5.Lcd.print("SSID missing");
    M5.Lcd.print("\n");
    return;
  }
  if (password=="") {
    M5.Lcd.print("Password missing");
    M5.Lcd.print("\n");
    return;
  }

  Serial.begin(115200);
  Serial.println("STA Connecting");

  StartNetwork.begin(ssid, password);
  delay(1000);
}

void loop() {
  M5.update();
  printHeader();
  if (M5.Lcd.getCursorY() > M5.Lcd.height()) {
    M5.Lcd.fillRect(0, HEADER_HEIGHT, 320, 240 - HEADER_HEIGHT, 0);
    M5.Lcd.setCursor(0, HEADER_HEIGHT);
    printWiFi();
  }

  if(StartNetwork.wifiConnected()){
    wifiConnectedLoop();
  }

  delay(1000);
}
