#include <SIM7020/SIM7020MqttClient.h>
#include <M5Atom.h>
#include <Arduino.h>

#define ST(A) #A
#define STR(A) ST(A)

const char apn[] = "telstra.iot";
//const char server[] = "mqtt001-0xacc5-dev.australiaeast.cloudapp.azure.com";
const char server[] = "mqtt001-0xacc5-dev-ipv4.australiaeast.cloudapp.azure.com";
const int16_t port = 8883;
const char mqtt_user[] = "mqttdevice1";
const char mqtt_password[] = STR(PIO_MQTT_PASSWORD);
const char version[] = STR(PIO_VERSION);


#define GSM_BAUDRATE 115200
#define GSM_TX_PIN 22
#define GSM_RX_PIN 19


#include <StreamDebugger.h>
StreamDebugger debugger(Serial1, Serial);
SIM7020GsmModem sim7020(debugger);
//SIM7020GsmModem sim7020(Serial1);

SIM7020TcpClient sim7020tcp(sim7020);
SIM7020MqttClient sim7020mqtt(sim7020tcp, server, port, true);

GsmModem& modem = sim7020;
MqttClient& mqtt = sim7020mqtt;

extern const uint8_t root_ca_pem_start[] asm("_binary_src_certs_ISRG_Root_X1_pem_start");
extern const uint8_t root_ca_pem_end[] asm("_binary_src_certs_ISRG_Root_X1_pem_end");
const String root_ca((char *)root_ca_pem_start);

const char publish_topic[] = "test/d2c";
const char subscribe_topic[] = "test/c2d";
String client_id;
char message_buffer[2000] = { 0 };
int32_t counter = 0;

bool ready = false;
bool failed = false;
int32_t led_off_ms = -1;
int32_t next_message_ms = -1;
int32_t disconnect_ms = -1;

// const char* message_template = \
//   "{\"battery_volts\":%.3f," \
//   "\"battery_amps\":%.3f," \
//   "\"battery_watts\":%.1f," \
//   "\"temperature_celsius\":%.2f}";

const char message_template[] = "{\"counter\":%d}";

void buildMessage() {
    counter++;
    snprintf(message_buffer, 2000, message_template, counter);
}

void setup() {
  AdvancedGsmLog.Log = &Serial;
  M5.begin(true, true, true);
  delay(1000);
  Serial.printf("MQTT example started, v.%s", version);
  M5.dis.fillpix(CRGB::Yellow); 
  Serial1.begin(GSM_BAUDRATE, SERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN);
  modem.begin(apn);
}

void loop() {
  int32_t now = millis();
  M5.update();
  modem.loop();

  if (failed) return;

  if (!modem.isActive()) {
    Serial.println("Fatal error - modem is not active");
    failed = true;
    M5.dis.fillpix(CRGB::Red); 
    return;
  }

  if (!ready) {
    if (modem.modemStatus() >= ModemStatus::PacketDataReady) {
      Serial.println("Modem is ready");
      ready = true;
      bool ca_success = modem.setRootCA(root_ca);
      if (!ca_success) {
        Serial.println("Certificate failed");
        failed = true;
        M5.dis.fillpix(CRGB::Red); 
        return;
      }
      client_id = "imei-" + modem.IMEI();
      Serial.printf("Setting client ID to: %s\n", client_id.c_str());
      M5.dis.fillpix(CRGB::Green);
      led_off_ms = now + 500;
      next_message_ms = now + 1000;
    }
    return;
  }

  if (led_off_ms > 0 && millis() > led_off_ms) {
    M5.dis.clear();
    led_off_ms = -1;
  }

  if (next_message_ms > 0 && millis() > next_message_ms) {
    int8_t rc = mqtt.connect(client_id.c_str(), mqtt_user, mqtt_password);
    if (rc != 0) {
      Serial.println("Connect failed");
      M5.dis.fillpix(CRGB::Red); 
      failed = true;
      return;
    }
    mqtt.subscribe(subscribe_topic);
    buildMessage();
    Serial.printf("Publishing: %s\n", message_buffer);
    mqtt.publish(publish_topic, message_buffer);
    M5.dis.fillpix(CRGB::Blue); 
    led_off_ms = now + 200;
    disconnect_ms = now + 5000;
    next_message_ms = now + 30000;
  }

  String receive_topic = mqtt.receiveTopic();
  if (receive_topic.length() > 0) {
    String receive_body = mqtt.receiveBody();
    Serial.printf("Received [%s]: %s\n", receive_topic.c_str(), receive_body.c_str());
    M5.dis.fillpix(CRGB::Blue); 
    led_off_ms = now + 200;
  }

  if (disconnect_ms > 0 && now > disconnect_ms) {
    mqtt.disconnect();
    disconnect_ms = -1;
  }
}
