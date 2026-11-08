/**
 * Perancangan Sistem Smart Lamp Berbasis ESP32 Menggunakan Sensor Suara
 * Terintegrasi Sensor DHT22, LED, dan Pushbutton
 * 
 * Program Studi S1 Informatika, Fakultas Teknik, Universitas Bengkulu
 * Laboratorium Jaringan Komputer & Sistem Tertanam
 * 
 * Pemenuhan Syarat Ceklis Verifikasi Praktikum:
 * 1. PlatformIO IDE + ESP32 Framework Arduino (firmware.bin zero error)
 * 2. Rangkaian ESP32 + DHT22 + LED + Pushbutton + Sensor Suara dengan keluaran JSON valid
 * 3. Demonstrasi respon tombol secara non-blocking (millis())
 * 4. Struktur 5 folder standar & riwayat commit git tim
 * 5. [C5] Payload JSON menggunakan pustaka ArduinoJson (JsonDocument)
 * 6. [C6] Tombol darurat mengubah interval telemetri 2000 ms (Normal) <-> 500 ms (Cepat)
 */

#include <Arduino.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include "config.h"

// Inisialisasi Objek Sensor DHT22
DHT dht(PIN_DHT, DHTTYPE);

// Status Lampu & Deteksi Suara
bool lampOn = false;
int clapCount = 0;
unsigned long firstClapTime = 0;
unsigned long lastClapTime = 0;
unsigned long statusOffTime = 0;
bool lastSoundState = LOW;

// Status Tombol & Mode Interval (C6)
bool isEmergencyMode = false;
int lastButtonState = HIGH;
unsigned long lastButtonTime = 0;

// Variabel Waktu Telemetri (Non-Blocking)
unsigned long previousTelemetryMillis = 0;
unsigned long telemetryInterval = INTERVAL_NORMAL; // Default: 2000 ms

/**
 * Mengubah status lampu utama (ON / OFF)
 */
void setLamp(bool state, const char *triggerSource) {
  lampOn = state;
  digitalWrite(PIN_LAMP, lampOn ? HIGH : LOW);

  // Buat event log instan dalam format JSON
  JsonDocument eventDoc;
  eventDoc["event"] = "lamp_toggle";
  eventDoc["lamp_status"] = lampOn ? "ON" : "OFF";
  eventDoc["trigger"] = triggerSource;
  eventDoc["timestamp_ms"] = millis();

  serializeJson(eventDoc, Serial);
  Serial.println();
}

/**
 * Menyalakan LED indikator status tepukan terdeteksi sejenak
 */
void flashStatus() {
  digitalWrite(PIN_STATUS, HIGH);
  statusOffTime = millis() + STATUS_BLINK_MS;
}

/**
 * Logika Deteksi Sensor Suara (Double Clap Non-Blocking)
 * Sistem mendeteksi 2 tepukan tangan dalam jendela waktu 1 detik.
 */
void handleSoundSensor() {
  unsigned long now = millis();

  // Suara terdeteksi dari pin digital DO sensor ATAU dari tombol simulasi CLAP di Wokwi
  bool soundDetected = (digitalRead(PIN_SOUND_DO) == HIGH) ||
                       (digitalRead(PIN_CLAP_SIM) == LOW);

  // Deteksi tepi naik (transisi suara baru) dengan jeda debounce
  if (soundDetected && !lastSoundState && (now - lastClapTime > CLAP_DEBOUNCE_MS)) {
    lastClapTime = now;
    flashStatus();

    // Jika tepukan sebelumnya sudah melewati batas jendela waktu 1 detik, reset hitungan
    if (clapCount > 0 && (now - firstClapTime > CLAP_WINDOW_MS)) {
      clapCount = 0;
    }

    clapCount++;

    if (clapCount == 1) {
      firstClapTime = now;
      int soundLevel = analogRead(PIN_SOUND_AO);

      JsonDocument clapDoc;
      clapDoc["event"] = "clap_detected";
      clapDoc["clap_number"] = 1;
      clapDoc["sound_analog"] = soundLevel;
      serializeJson(clapDoc, Serial);
      Serial.println();
    } else if (clapCount == 2) {
      // Dua tepukan dalam rentang waktu < 1 detik -> Toggle Lampu!
      setLamp(!lampOn, "double_clap");
      clapCount = 0;
    }
  }
  lastSoundState = soundDetected;

  // Jika hanya 1 tepukan dan jendela waktu habis, reset hitungan
  if (clapCount == 1 && (now - firstClapTime > CLAP_WINDOW_MS)) {
    clapCount = 0;
  }
}

/**
 * Penanganan Tombol Manual / Mode Darurat (Non-Blocking)
 * Memenuhi Ceklis 3 & Soal C6:
 * - Menggunakan non-blocking polling di loop() tanpa delay()
 * - Mengubah interval telemetri antara 2000 ms (Normal) dan 500 ms (Cepat)
 */
void handleButton() {
  int reading = digitalRead(PIN_BUTTON);

  if (reading == LOW && lastButtonState == HIGH &&
      (millis() - lastButtonTime > BUTTON_DEBOUNCE)) {
    lastButtonTime = millis();

    // Toggle mode interval (Soal C6)
    isEmergencyMode = !isEmergencyMode;
    telemetryInterval = isEmergencyMode ? INTERVAL_CEPAT : INTERVAL_NORMAL;

    // Toggle lampu sebagai saklar manual cadangan
    setLamp(!lampOn, "manual_button");

    JsonDocument btnDoc;
    btnDoc["event"] = "button_pressed";
    btnDoc["mode"] = isEmergencyMode ? "DARURAT (500ms)" : "NORMAL (2000ms)";
    btnDoc["interval_ms"] = telemetryInterval;
    serializeJson(btnDoc, Serial);
    Serial.println();
  }
  lastButtonState = reading;
}

/**
 * Pengiriman Telemetri Periodik (JSON Valid via ArduinoJson)
 * Memenuhi Ceklis 2 & Soal C5
 */
void sendPeriodicTelemetry() {
  float suhu = dht.readTemperature();
  float kelembaban = dht.readHumidity();
  int soundLevel = analogRead(PIN_SOUND_AO);

  // Jika DHT gagal dibaca, set nilai default agar JSON tetap valid
  if (isnan(suhu)) suhu = 0.0;
  if (isnan(kelembaban)) kelembaban = 0.0;

  // Membangun payload JSON menggunakan ArduinoJson (JsonDocument)
  JsonDocument doc;
  doc["system"] = "SmartLamp_ESP32";
  doc["lamp_status"] = lampOn ? "ON" : "OFF";
  doc["suhu"] = serialized(String(suhu, 2));
  doc["kelembaban"] = serialized(String(kelembaban, 2));
  doc["sound_level"] = soundLevel;
  doc["mode"] = isEmergencyMode ? "DARURAT" : "NORMAL";
  doc["interval_ms"] = telemetryInterval;
  doc["uptime_ms"] = millis();

  // Serialisasi ke Serial Monitor
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
  Serial.println(" Tepuk tangan 2x (atau tekan tombol CLAP merah) untuk saklar lampu.");
  Serial.println(" Tekan tombol biru untuk saklar manual & ubah interval (C6).");
  Serial.println(" Output Telemetri: JSON Valid (ArduinoJson)");
  Serial.println("==========================================================");
}

void loop() {
  // 1. Tangani sensor suara secara non-blocking setiap iterasi
  handleSoundSensor();

  // 2. Tangani tombol manual secara non-blocking setiap iterasi
  handleButton();

  // 3. Matikan LED indikator status setelah durasi kedip selesai
  if (statusOffTime && millis() >= statusOffTime) {
    digitalWrite(PIN_STATUS, LOW);
    statusOffTime = 0;
  }

  // 4. Kirim telemetri periodik secara non-blocking (millis())
  unsigned long currentMillis = millis();
  if (currentMillis - previousTelemetryMillis >= telemetryInterval) {
    previousTelemetryMillis = currentMillis;
    sendPeriodicTelemetry();
  }
}
