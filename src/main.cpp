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
#include "config.h"

// Inisialisasi Objek Sensor DHT22
DHT dht(PIN_DHT, DHTTYPE);

// Status Lampu & LED
bool lampOn = false;
unsigned long statusOffTime = 0;

// Logika Deteksi Tepukan (Double Clap)
int clapCount = 0;
unsigned long firstClapTime = 0;
unsigned long lastClapTime = 0;

// Tracking State Independen untuk Tombol CLAP (Pin 13)
int lastClapBtnState = HIGH;
unsigned long lastClapBtnDebounce = 0;

// Tracking State Independen untuk Sensor Suara DO (Pin 27)
int lastMicState = LOW;
unsigned long lastMicDebounce = 0;

// Status Tombol Manual / Mode Darurat (Pin 14)
bool isEmergencyMode = false;
int lastManualBtnState = HIGH;
unsigned long lastManualBtnDebounce = 0;

// Variabel Waktu Telemetri (Non-Blocking)
unsigned long previousTelemetryMillis = 0;
unsigned long telemetryInterval = INTERVAL_NORMAL; // Default 2000 ms

/**
 * Mengubah status lampu utama (LED Kuning di GPIO 26)
 */
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

/**
 * Menyalakan LED indikator status tepukan (LED Hijau di GPIO 25)
 */
void flashStatus() {
  digitalWrite(PIN_STATUS, HIGH);
  statusOffTime = millis() + STATUS_BLINK_MS;
}

/**
 * Fungsi Perekam Tepukan (Baik dari Tombol CLAP maupun Sensor Suara)
 * Memerlukan 2 tepukan dalam rentang waktu < 1.2 detik (Double Clap).
 */
void registerClap(const char *source) {
  unsigned long now = millis();
  flashStatus(); // LED hijau langsung menyala

  // Jika tepukan sebelumnya sudah melewati batas jendela waktu, reset
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

    // Toggle status lampu utama
    setLamp(!lampOn, source);
    clapCount = 0;
  }
}

/**
 * Penanganan Tombol Simulasi Tepukan (CLAP Button Merah di GPIO 13)
 * Menggunakan deteksi tepi independen (bebas dari pengaruh pin sensor suara)
 */
void handleClapButton() {
  int reading = digitalRead(PIN_CLAP_SIM);

  // Deteksi transisi dari HIGH (lepas) ke LOW (ditekan)
  if (reading == LOW && lastClapBtnState == HIGH &&
      (millis() - lastClapBtnDebounce > CLAP_DEBOUNCE_MS)) {
    lastClapBtnDebounce = millis();
    registerClap("tombol_clap_merah");
  }
  lastClapBtnState = reading;
}

/**
 * Penanganan Sensor Suara Fisik (DO di GPIO 27)
 */
void handleMicrophoneDO() {
  int reading = digitalRead(PIN_SOUND_DO);

  // Deteksi transisi suara
  if (reading != lastMicState) {
    if (reading == HIGH && (millis() - lastMicDebounce > CLAP_DEBOUNCE_MS)) {
      lastMicDebounce = millis();
      registerClap("sensor_suara_DO");
    }
    lastMicState = reading;
  }
}

/**
 * Reset otomatis jika hanya 1 tepukan dan jendela waktu habis
 */
void handleClapTimeout() {
  if (clapCount == 1 && (millis() - firstClapTime > CLAP_WINDOW_MS)) {
    Serial.println(">> [CLAP INFO] Batas waktu habis (hanya 1 tepukan). Hitungan di-reset.");
    clapCount = 0;
  }
}

/**
 * Penanganan Tombol Manual / Mode Darurat (Tombol Biru di GPIO 14)
 * Memenuhi Ceklis 3 & Soal C6:
 * - Mengubah interval telemetri antara 2000 ms (Normal) dan 500 ms (Cepat)
 * - Berfungsi sebagai saklar manual lampu
 */
void handleManualButton() {
  int reading = digitalRead(PIN_BUTTON);

  if (reading == LOW && lastManualBtnState == HIGH &&
      (millis() - lastManualBtnDebounce > BUTTON_DEBOUNCE)) {
    lastManualBtnDebounce = millis();

    // Toggle mode interval (Soal C6)
    isEmergencyMode = !isEmergencyMode;
    telemetryInterval = isEmergencyMode ? INTERVAL_CEPAT : INTERVAL_NORMAL;

    // Toggle lampu manual
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

/**
 * Pengiriman Telemetri Periodik (JSON Valid via ArduinoJson)
 * Memenuhi Ceklis 2 & Soal C5
 */
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

  // Konfigurasi pin input & output
  pinMode(PIN_SOUND_DO, INPUT);
  pinMode(PIN_SOUND_AO, INPUT);
  pinMode(PIN_LAMP, OUTPUT);
  pinMode(PIN_STATUS, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_CLAP_SIM, INPUT_PULLUP);

  digitalWrite(PIN_LAMP, LOW);
  digitalWrite(PIN_STATUS, LOW);

  // Inisialisasi sensor suhu & kelembaban DHT22
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
  // 1. Tangani tombol CLAP merah secara responsif
  handleClapButton();

  // 2. Tangani sensor suara DO
  handleMicrophoneDO();

  // 3. Tangani batas waktu tepukan tunggal
  handleClapTimeout();

  // 4. Tangani tombol manual biru
  handleManualButton();

  // 5. Matikan LED indikator status hijau setelah durasi kedip selesai
  if (statusOffTime && millis() >= statusOffTime) {
    digitalWrite(PIN_STATUS, LOW);
    statusOffTime = 0;
  }

  // 6. Kirim telemetri periodik secara non-blocking (millis())
  unsigned long currentMillis = millis();
  if (currentMillis - previousTelemetryMillis >= telemetryInterval) {
    previousTelemetryMillis = currentMillis;
    sendPeriodicTelemetry();
  }
}
