// export PIO_MQTT_PASSWORD=YourMqttPassword3
// (export PIO_VERSION=$(git describe --tags --dirty); pio run --target upload)
// pio device monitor --baud 115200

#include <Arduino.h>
#include <M5Atom.h>
#include <M5_ENV.h>
#include <SIM7020/SIM7020MqttClient.h>

#define ST(A) #A
#define STR(A) ST(A)

const char apn[] = "telstra.iot";

// NOTE: Maximum server name length is 50 characters
const char server[] = "mqdev01-0xacc5.australiaeast.cloudapp.azure.com";
const int16_t port = 8883;
const char mqtt_user[] = "dev00001";
const char mqtt_password[] = STR(PIO_MQTT_PASSWORD);

const char manufacturer[] = "M5Stack";
const char model[] = "Atom Lite; IoT Demo";
const char version[] = STR(PIO_VERSION);

#define GSM_BAUDRATE 115200
#define GSM_TX_PIN 22
#define GSM_RX_PIN 19

// #include <StreamDebugger.h>
// StreamDebugger debugger(Serial1, Serial);
// SIM7020GsmModem sim7020(debugger);
SIM7020GsmModem sim7020(Serial1);

SIM7020TcpClient sim7020tcp(sim7020);
SIM7020MqttClient sim7020mqtt(sim7020tcp, server, port, true);

GsmModem &modem = sim7020;
MqttClient &mqtt = sim7020mqtt;

extern const uint8_t
    root_ca_pem_start[] asm("_binary_src_certs_ISRG_Root_X1_pem_start");
extern const uint8_t
    root_ca_pem_end[] asm("_binary_src_certs_ISRG_Root_X1_pem_end");
const String root_ca((char *)root_ca_pem_start);

const char publish_topic[] = "dt/demo/m5/dev00001/senml";
const char subscribe_topic[] = "cmd/demo/m5/dev00001/#";
char message_buffer[2000] = {0};

bool ready = false;
bool failed = false;
bool send_properties = true;
int32_t led_off_ms = -1;
int32_t next_message_ms = -1;
int32_t disconnect_ms = -1;

SHT3X sht30;
QMP6988 qmp6988;

// First message (or on click?)
// vs = manufacturer, model, swVersion, imei, ip
// rssi dBW, batteryVoltage V

const char properties_template[] =
    "[{\"bn\":\"%s_\",\"n\":\"manufacturer\",\"vs\":\"%s\"},"
    "{\"n\":\"model\",\"vs\":\"%s\"},"
    "{\"n\":\"swVersion\",\"vs\":\"%s\"},"
    "{\"n\":\"imei\",\"vs\":\"%s\"},"
    "{\"n\":\"ipAddress\",\"vs\":\"%s\"}]";

void buildPropertiesMessage() {
  String imei = modem.IMEI();
  String ip_addresses[4]; // The SIM7020 may get up to 4 addresses
  modem.getLocalIPs(ip_addresses, 4);
  int32_t rssidBm = modem.RSSI();
  snprintf(message_buffer, 2000, properties_template, mqtt_user, manufacturer,
           model, version, imei.c_str(), ip_addresses[0].c_str());
}

const char telemetry_template[] =
    "[{\"bn\":\"%s_\",\"n\":\"temperature\",\"u\":\"Cel\",\"v\":%.2f},"
    "{\"n\":\"humidity\",\"u\":\"%RH\",\"v\":%.1f},"
    "{\"n\":\"pressure\",\"u\":\"Pa\",\"v\":%.0f},"
    "{\"n\":\"rssi\",\"u\":\"dBW\",\"v\":%.3f},"
    "{\"n\":\"batteryVoltage\",\"u\":\"V\",\"v\":%.3f}]";

void buildTelemetryMessage() {
  float temperature_celsius = 0.0;
  float humidity_percent = 0.0;
  float pressure_pascal = 0.0;
  pressure_pascal = qmp6988.calcPressure(); // standard atmosphere is 101325 Pa
  if (sht30.get() == 0) {                   // Obtain the data of SHT30.
    temperature_celsius =
        sht30.cTemp; // Store the temperature obtained from SHT30.
    humidity_percent =
        sht30.humidity; // Store the humidity obtained from the SHT30.
  } else {
    temperature_celsius = 0;
    humidity_percent = 0;
  }
  int32_t rssi_dBm = modem.RSSI();
  float rssi_dBW = rssi_dBm / 1000.0f;
  float battery_voltage = 0.0;
  snprintf(message_buffer, 2000, telemetry_template, mqtt_user,
           temperature_celsius, humidity_percent, pressure_pascal, rssi_dBW,
           battery_voltage);
}

void setup() {
  AdvancedGsmLog.Log = &Serial;
  M5.begin(true, true, true);
  Wire.begin(26, 32); // Initialize pin 26,32.  初始化26,32引脚
  qmp6988.init();
  delay(1000);
  Serial.printf("MQTT example started, v.%s\n", version);
  M5.dis.fillpix(CRGB::Yellow);
  Serial1.begin(GSM_BAUDRATE, SERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN);
  modem.begin(apn);
}

void loop() {
  int32_t now = millis();
  M5.update();
  modem.loop();

  if (failed)
    return;

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
      String ip_addresses[4];
      modem.getLocalIPs(ip_addresses, 4);
      Serial.printf("Device IP address: %s\n", ip_addresses[0].c_str());
      bool ca_success = modem.setRootCA(root_ca);
      if (!ca_success) {
        Serial.println("Certificate failed");
        failed = true;
        M5.dis.fillpix(CRGB::Red);
        return;
      }
      M5.dis.fillpix(CRGB::Green);
      led_off_ms = millis() + 500;
      next_message_ms = millis() + 1000;
    }
    return;
  }

  if (led_off_ms > 0 && millis() > led_off_ms) {
    M5.dis.clear();
    led_off_ms = -1;
  }

  if (M5.Btn.wasPressed()) {
    send_properties = true;
    next_message_ms = 1;
  }

  if (next_message_ms > 0 && millis() > next_message_ms) {
    M5.dis.fillpix(CRGB::Blue);

    Serial.println("Connecting");
    int8_t rc = mqtt.connect(mqtt_user, mqtt_user, mqtt_password);
    if (rc != 0) {
      Serial.println("Connect failed");
      M5.dis.fillpix(CRGB::Red);
      failed = true;
      return;
    }

    Serial.printf("Subscribing to %s\n", subscribe_topic);
    mqtt.subscribe(subscribe_topic);

    if (send_properties) {
      buildPropertiesMessage();
      Serial.printf("Publishing: %s\n", message_buffer);
      mqtt.publish(publish_topic, message_buffer);
      send_properties = false;
    }

    buildTelemetryMessage();
    Serial.printf("Publishing: %s\n", message_buffer);
    mqtt.publish(publish_topic, message_buffer);

    led_off_ms = millis() + 200;
    disconnect_ms = millis() + 5000;
    next_message_ms = now + 60000;
  }

  String receive_topic = mqtt.receiveTopic();
  if (receive_topic.length() > 0) {
    String receive_body = mqtt.receiveBody();
    Serial.printf("Received [%s]: %s\n", receive_topic.c_str(),
                  receive_body.c_str());
    M5.dis.fillpix(CRGB::Blue);
    led_off_ms = now + 200;
  }

  if (disconnect_ms > 0 && now > disconnect_ms) {
    Serial.print("Disconnecting\n");
    mqtt.disconnect();
    disconnect_ms = -1;
  }
}
