/**
 * Perancangan Sistem Smart Lamp Berbasis ESP32 Menggunakan Sensor Suara
 * Terintegrasi Sensor DHT22, LED, dan Pushbutton
 * 
 * Program Studi S1 Informatika, Fakultas Teknik, Universitas Bengkulu
 * Laboratorium Jaringan Komputer & Sistem Tertanam
 */

#include <Arduino.h>
#include <DHT.h>
#include <ArduinoJson.h>

// --- Konfigurasi Pin Hardware ---
#define PIN_SOUND_DO       27   // Digital Output sensor suara
#define PIN_SOUND_AO       34   // Analog Output sensor suara
#define PIN_DHT            15   // Pin data DHT22 (Ceklis 2)
#define DHTTYPE            DHT22
#define PIN_LAMP           26   // LED Lampu Utama (Kuning)
#define PIN_STATUS         25   // LED Indikator Tepukan (Hijau)
#define PIN_BUTTON         14   // Tombol Manual / Mode Cepat (Ceklis 2 & C6)
#define PIN_CLAP_SIM       13   // Tombol Simulasi Tepukan Wokwi (CLAP)

// --- Parameter Waktu ---
#define CLAP_DEBOUNCE_MS   120  // Anti-bouncing tepukan (ms)
#define CLAP_WINDOW_MS     1200 // Batas waktu tepukan ke-2 (1.2 detik)
#define STATUS_BLINK_MS    100  // Lama kedip LED status
#define INTERVAL_NORMAL    2000 // Mode Normal: 2000 ms
#define INTERVAL_CEPAT     500  // Mode Cepat: 500 ms
#define BUTTON_DEBOUNCE    50   // Debounce tombol manual

DHT dht(PIN_DHT, DHTTYPE);

bool lampOn = false;
unsigned long statusOffTime = 0;

int clapCount = 0;
unsigned long firstClapTime = 0;

// Tracking state independen
int lastClapBtnState = HIGH;
unsigned long lastClapBtnDebounce = 0;

int lastMicState = LOW;
unsigned long lastMicDebounce = 0;

bool isEmergencyMode = false;
int lastManualBtnState = HIGH;
unsigned long lastManualBtnDebounce = 0;

unsigned long previousTelemetryMillis = 0;
unsigned long telemetryInterval = INTERVAL_NORMAL;

void setLamp(bool state, const char *triggerSource) {
  lampOn = state;
  digitalWrite(PIN_LAMP, lampOn ? HIGH : LOW);

  Serial.println("--------------------------------------------------");
  Serial.printf(">> [LAMPU] Status: %s | Sumber: %s\n", lampOn ? "MENYALA (ON)" : "PADAM (OFF)", triggerSource);
  Serial.println("--------------------------------------------------");

  JsonDocument eventDoc;
  eventDoc["event"] = "lamp_toggle";
  eventDoc["lamp_status"] = lampOn ? "ON" : "OFF";
  eventDoc["trigger"] = triggerSource;
  eventDoc["timestamp_ms"] = millis();

  serializeJson(eventDoc, Serial);
  Serial.println();
}

void flashStatus() {
  digitalWrite(PIN_STATUS, HIGH);
  statusOffTime = millis() + STATUS_BLINK_MS;
}

void registerClap(const char *source) {
  unsigned long now = millis();
  flashStatus();

  if (clapCount > 0 && (now - firstClapTime > CLAP_WINDOW_MS)) {
    clapCount = 0;
  }

  clapCount++;

  if (clapCount == 1) {
    firstClapTime = now;
    int soundLevel = analogRead(PIN_SOUND_AO);

    Serial.println();
    Serial.printf(">> [CLAP 1] Tepukan 1 terdeteksi (%s)! Level: %d\n", source, soundLevel);
    Serial.println("   --> KLIK 1 KALI LAGI sekarang untuk menyalakan/mematikan lampu! <--");

    JsonDocument clapDoc;
    clapDoc["event"] = "clap_1";
    clapDoc["source"] = source;
    clapDoc["sound_analog"] = soundLevel;
    clapDoc["message"] = "Tepukan 1 diterima, menunggu tepukan ke-2";
    serializeJson(clapDoc, Serial);
    Serial.println();

  } else if (clapCount >= 2) {
    unsigned long intervalClap = now - firstClapTime;
    Serial.println();
    Serial.printf(">> [CLAP 2] DOUBLE CLAP BERHASIL (%s)! Selang waktu: %lu ms.\n", source, intervalClap);

    setLamp(!lampOn, source);
    clapCount = 0;
  }
}

void handleClapButton() {
  int reading = digitalRead(PIN_CLAP_SIM);

  if (reading == LOW && lastClapBtnState == HIGH &&
      (millis() - lastClapBtnDebounce > CLAP_DEBOUNCE_MS)) {
    lastClapBtnDebounce = millis();
    registerClap("tombol_clap_merah");
  }
  lastClapBtnState = reading;
}

void handleMicrophoneDO() {
  int reading = digitalRead(PIN_SOUND_DO);

  if (reading != lastMicState) {
    if (reading == HIGH && (millis() - lastMicDebounce > CLAP_DEBOUNCE_MS)) {
      lastMicDebounce = millis();
      registerClap("sensor_suara_DO");
    }
    lastMicState = reading;
  }
}

void handleClapTimeout() {
  if (clapCount == 1 && (millis() - firstClapTime > CLAP_WINDOW_MS)) {
    Serial.println(">> [CLAP INFO] Batas waktu habis (hanya 1 tepukan). Hitungan di-reset.");
    clapCount = 0;
  }
}

void handleManualButton() {
  int reading = digitalRead(PIN_BUTTON);

  if (reading == LOW && lastManualBtnState == HIGH &&
      (millis() - lastManualBtnDebounce > BUTTON_DEBOUNCE)) {
    lastManualBtnDebounce = millis();

    isEmergencyMode = !isEmergencyMode;
    telemetryInterval = isEmergencyMode ? INTERVAL_CEPAT : INTERVAL_NORMAL;

    setLamp(!lampOn, "tombol_manual_biru");

    Serial.printf(">> [MODE TELEMETRI] Beralih ke: %s (%lu ms)\n",
                  isEmergencyMode ? "CEPAT / DARURAT" : "NORMAL", telemetryInterval);

    JsonDocument btnDoc;
    btnDoc["event"] = "button_mode_changed";
    btnDoc["mode"] = isEmergencyMode ? "DARURAT" : "NORMAL";
    btnDoc["interval_ms"] = telemetryInterval;
    serializeJson(btnDoc, Serial);
    Serial.println();
  }
  lastManualBtnState = reading;
}

void sendPeriodicTelemetry() {
  float suhu = dht.readTemperature();
  float kelembaban = dht.readHumidity();
  int soundLevel = analogRead(PIN_SOUND_AO);

  if (isnan(suhu)) suhu = 0.0;
  if (isnan(kelembaban)) kelembaban = 0.0;

  JsonDocument doc;
  doc["system"] = "SmartLamp_ESP32";
  doc["lamp_status"] = lampOn ? "ON" : "OFF";
  doc["suhu"] = serialized(String(suhu, 2));
  doc["kelembaban"] = serialized(String(kelembaban, 2));
  doc["sound_level"] = soundLevel;
  doc["mode"] = isEmergencyMode ? "DARURAT" : "NORMAL";
  doc["interval_ms"] = telemetryInterval;
  doc["uptime_ms"] = millis();

  serializeJson(doc, Serial);
  Serial.println();
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_SOUND_DO, INPUT);
  pinMode(PIN_SOUND_AO, INPUT);
  pinMode(PIN_LAMP, OUTPUT);
  pinMode(PIN_STATUS, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_CLAP_SIM, INPUT_PULLUP);

  digitalWrite(PIN_LAMP, LOW);
  digitalWrite(PIN_STATUS, LOW);

  dht.begin();

  Serial.println("==========================================================");
  Serial.println(" Perancangan Sistem Smart Lamp Berbasis ESP32 (Sensor Suara)");
  Serial.println(" Terintegrasi: Sensor Suara + DHT22 + LED Lampu + Tombol");
  Serial.println(" 1. Klik tombol merah 'CLAP' 2 KALI CEPAT untuk nyalakan lampu.");
  Serial.println(" 2. Klik tombol biru untuk saklar manual & ubah interval (C6).");
  Serial.println(" Output Telemetri: JSON Valid (ArduinoJson)");
  Serial.println("==========================================================");
}

void loop() {
  handleClapButton();
  handleMicrophoneDO();
  handleClapTimeout();
  handleManualButton();

  if (statusOffTime && millis() >= statusOffTime) {
    digitalWrite(PIN_STATUS, LOW);
    statusOffTime = 0;
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousTelemetryMillis >= telemetryInterval) {
    previousTelemetryMillis = currentMillis;
    sendPeriodicTelemetry();
  }
}
