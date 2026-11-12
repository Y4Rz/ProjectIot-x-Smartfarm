#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

const char* WIFI_SSID = "Kost";
const char* WIFI_PASSWORD = "jangandikasihtau";
const char* MQTT_HOST = "broker.hivemq.com";
const uint16_t MQTT_PORT = 1883;
const char* TOPIC_STATUS = "pertanian/hama/status";
const char* TOPIC_CONTROL = "pertanian/hama/kontrol";

constexpr uint8_t PIN_TRIG = 25;
constexpr uint8_t PIN_ECHO = 26;
constexpr uint8_t PIN_PIR = 27;
constexpr uint8_t PIN_BUZZER = 14;
constexpr uint8_t PIN_LED = 13;
constexpr uint8_t PIN_IR = 32;
constexpr uint8_t PIN_SERVO = 33;
constexpr uint8_t PIN_SENSE = 34;  // feedback arus buzzer

constexpr float DETECT_DISTANCE_CM = 50.0f;
constexpr int SENSE_THRESHOLD = 500;  // ambang ADC 12 bit, kalibrasi lewat Serial
constexpr int SERVO_REST = 90, SERVO_A = 60, SERVO_B = 120;
constexpr unsigned long SENSOR_PERIOD_MS = 250;
constexpr unsigned long PUBLISH_PERIOD_MS = 1000;
constexpr unsigned long RECONNECT_PERIOD_MS = 5000;
constexpr unsigned long BUZZER_TEST_PERIOD_MS = 2000;
constexpr unsigned long STRIKE_PERIOD_MS = 300;

enum Status { AMAN, WASPADA, BAHAYA };
const char* STATUS_TEXT[] = {"AMAN", "WASPADA", "BAHAYA"};

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
Servo bellServo;

bool manualMode = false, manualBuzzer = false, manualLed = false;
bool manualIr = false, manualBell = false;
bool motionDetected = false, buzzerFault = false, prevWantBuzzer = false;
bool strikeToggle = false, bellActive = false;
Status status = AMAN;
float distanceCm = -1.0f;
unsigned long lastSensorMs = 0, lastPublishMs = 0, lastReconnectMs = 0, lastWifiMs = 0;
unsigned long lastBuzzerTestMs = 0, lastStrikeMs = 0;

bool wantBuzzer() { return manualMode ? manualBuzzer : status == BAHAYA; }
bool wantLed()    { return manualMode ? manualLed    : status == BAHAYA; }
bool wantIr()     { return manualMode ? manualIr     : status != AMAN; }
bool wantBell()   { return manualMode ? manualBell   : (status == BAHAYA && buzzerFault); }

// Uji rangkaian buzzer: alirkan arus 5 ms, ukur tegangan di resistor 100 ohm
bool testBuzzerCircuit() {
  noTone(PIN_BUZZER);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, HIGH);
  delay(5);
  int peak = 0;
  for (int i = 0; i < 5; i++) peak = max(peak, (int)analogRead(PIN_SENSE));
  digitalWrite(PIN_BUZZER, LOW);
  Serial.printf("[UJI BUZZER] puncak ADC=%d\n", peak);
  return peak > SENSE_THRESHOLD;
}

void applyOutputs() {
  digitalWrite(PIN_LED, wantLed() ? HIGH : LOW);
  digitalWrite(PIN_IR, wantIr() ? HIGH : LOW);
  if (wantBuzzer()) {
    const unsigned long now = millis();
    if (!prevWantBuzzer || now - lastBuzzerTestMs >= BUZZER_TEST_PERIOD_MS) {
      lastBuzzerTestMs = now;
      buzzerFault = !testBuzzerCircuit();
    }
    if (!buzzerFault) tone(PIN_BUZZER, 4000);
  } else {
    noTone(PIN_BUZZER);
    digitalWrite(PIN_BUZZER, LOW);
  }
  prevWantBuzzer = wantBuzzer();
}

void updateBell() {
  if (wantBell()) {
    const unsigned long now = millis();
    if (now - lastStrikeMs >= STRIKE_PERIOD_MS) {
      lastStrikeMs = now;
      strikeToggle = !strikeToggle;
      bellServo.write(strikeToggle ? SERVO_A : SERVO_B);
    }
    bellActive = true;
  } else if (bellActive) {
    bellServo.write(SERVO_REST);
    bellActive = false;
  }
}

float readDistanceCm() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(3);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  const unsigned long duration = pulseIn(PIN_ECHO, HIGH, 30000UL);
  if (duration == 0) return -1.0f;
  return (duration * 0.0343f) / 2.0f;
}

String jsonValue(const char* json, const char* key) {
  String needle = String("\"") + key + "\"";
  const char* start = strstr(json, needle.c_str());
  if (!start) return "";
  start += needle.length();
  while (*start && (*start == ' ' || *start == ':')) start++;
  if (*start != '\"') return "";
  start++;
  const char* end = strchr(start, '\"');
  if (!end) return "";
  String value;
  while (start < end) value += *start++;
  value.toUpperCase();
  return value;
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, TOPIC_CONTROL) != 0 || length >= 192) return;
  char message[192];
  memcpy(message, payload, length);
  message[length] = '\0';
  Serial.print("[MQTT IN] ");
  Serial.println(message);

  const String mode = jsonValue(message, "mode");
  if (mode == "MANUAL") manualMode = true;
  else if (mode == "AUTO") manualMode = false;

  const String buzzer = jsonValue(message, "buzzer");
  const String led = jsonValue(message, "led");
  const String ir = jsonValue(message, "ir");
  const String bell = jsonValue(message, "lonceng");
  if (buzzer.length() || led.length() || ir.length() || bell.length()) manualMode = true;
  if (buzzer == "ON") manualBuzzer = true; else if (buzzer == "OFF") manualBuzzer = false;
  if (led == "ON") manualLed = true; else if (led == "OFF") manualLed = false;
  if (ir == "ON") manualIr = true; else if (ir == "OFF") manualIr = false;
  if (bell == "ON") manualBell = true; else if (bell == "OFF") manualBell = false;
  applyOutputs();
}

bool connectMqtt() {
  String clientId = "SmartHama-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  Serial.print("Menghubungkan ke MQTT...");
  if (!mqtt.connect(clientId.c_str())) {
    Serial.println(" Gagal!");
    return false;
  }
  Serial.println(" Terhubung!");
  mqtt.subscribe(TOPIC_CONTROL);
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- Smart Hama ---");

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_PIR, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_IR, OUTPUT);
  pinMode(PIN_SENSE, INPUT);
  analogSetPinAttenuation(PIN_SENSE, ADC_11db);
  digitalWrite(PIN_TRIG, LOW);
  digitalWrite(PIN_LED, LOW);
  digitalWrite(PIN_IR, LOW);
  digitalWrite(PIN_BUZZER, LOW);

  ESP32PWM::allocateTimer(0);
  bellServo.setPeriodHertz(50);
  bellServo.attach(PIN_SERVO, 500, 2400);
  bellServo.write(SERVO_REST);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Menghubungkan ke Wi-Fi...");

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  mqtt.setBufferSize(256);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastWifiMs >= RECONNECT_PERIOD_MS) {
      lastWifiMs = millis();
      Serial.print(".");
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }
  } else if (!mqtt.connected() && millis() - lastReconnectMs >= RECONNECT_PERIOD_MS) {
    lastReconnectMs = millis();
    connectMqtt();
  }
  if (mqtt.connected()) mqtt.loop();

  const unsigned long now = millis();
  if (now - lastSensorMs >= SENSOR_PERIOD_MS) {
    lastSensorMs = now;
    motionDetected = digitalRead(PIN_PIR) == HIGH;
    distanceCm = readDistanceCm();
    if (!motionDetected) status = AMAN;
    else if (distanceCm > 0.0f && distanceCm <= DETECT_DISTANCE_CM) status = BAHAYA;
    else status = WASPADA;
    if (!manualMode) applyOutputs();
  }

  updateBell();

  if (mqtt.connected() && now - lastPublishMs >= PUBLISH_PERIOD_MS) {
    lastPublishMs = now;
    char jarak[12];
    if (distanceCm < 0.0f) strcpy(jarak, "null");
    else snprintf(jarak, sizeof(jarak), "%.1f", distanceCm);
    char payload[192];
    snprintf(payload, sizeof(payload),
      "{\"gerakan\":%s,\"jarak_cm\":%s,\"status\":\"%s\",\"alarm\":\"%s\",\"ir\":\"%s\",\"buzzer_ok\":%s,\"lonceng\":\"%s\"}",
      motionDetected ? "true" : "false", jarak, STATUS_TEXT[status],
      (wantBuzzer() && !buzzerFault) ? "ACTIVE" : "OFF", wantIr() ? "ON" : "OFF",
      buzzerFault ? "false" : "true", wantBell() ? "ON" : "OFF");
    mqtt.publish(TOPIC_STATUS, payload);
    Serial.print("[PUBLISH] ");
    Serial.println(payload);
  }
}
