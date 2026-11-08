/**
 * Praktikum Minggu 1 - Teknologi IoT (Lab-0: Toolchain & Wokwi Simulator)
 * Program Studi S1 Informatika, Fakultas Teknik, Universitas Bengkulu
 * 
 * Eksperimen 3 & Tugas C6 (Mencipta):
 * - Membaca sensor suhu dan kelembaban DHT22.
 * - Mengirimkan payload telemetri berformat JSON valid ke Serial Monitor (ArduinoJson).
 * - Tombol darurat non-blocking dengan debouncing:
 *   Mengubah interval pengiriman dari 2000 ms (Normal) menjadi 500 ms (Darurat/Cepat),
 *   dan kembali ke 2000 ms jika ditekan lagi.
 * - Indikator LED menyala singkat setiap kali telemetri terkirim.
 */

#include <Arduino.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include "config.h"

// Inisialisasi Objek Sensor DHT
DHT dht(PIN_DHT, DHTTYPE);

// Variabel Waktu Telemetri (Non-Blocking menggunakan millis())
unsigned long previousMillis = 0;
unsigned long intervalTelemetri = INTERVAL_NORMAL;

// Variabel Tombol Darurat & Debounce
bool isEmergencyMode = false;
int buttonState = HIGH;
int lastButtonReading = HIGH;
unsigned long lastDebounceTime = 0;

/**
 * Fungsi Penanganan Tombol Darurat (Non-Blocking)
 * Berbeda dengan metode blocking (delay), metode ini dipanggil pada setiap iterasi
 * loop() sehingga penekanan tombol dideteksi secara instan tanpa ada event yang terlewat.
 */
void handleEmergencyButton() {
  int currentReading = digitalRead(PIN_BUTTON);

  // Jika kondisi pembacaan berubah, catat waktu debounce
  if (currentReading != lastButtonReading) {
    lastDebounceTime = millis();
  }

  // Jika pembacaan stabil melampaui jeda debounce
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (currentReading != buttonState) {
      buttonState = currentReading;

      // Terdeteksi transisi HIGH ke LOW (tombol ditekan ke GND)
      if (buttonState == LOW) {
        // [C6 - Mencipta] Toggle interval antara 2000 ms dan 500 ms
        isEmergencyMode = !isEmergencyMode;
        intervalTelemetri = isEmergencyMode ? INTERVAL_CEPAT : INTERVAL_NORMAL;

        Serial.print(">> [ALERT] Tombol darurat ditekan! Mode beralih ke: ");
        Serial.print(isEmergencyMode ? "DARURAT (500 ms)" : "NORMAL (2000 ms)");
        Serial.println(" <<");
      }
    }
  }
  lastButtonReading = currentReading;
}

/**
 * Fungsi Pengiriman Telemetri JSON
 * Membangun payload JSON menggunakan ArduinoJson (JsonDocument)
 * sehingga aman terhadap buffer overflow dan mudah dipelihara (memenuhi C5).
 */
void sendTelemetry() {
  float suhu = dht.readTemperature();
  float kelembaban = dht.readHumidity();

  // Validasi pembacaan sensor
  if (isnan(suhu) || isnan(kelembaban)) {
    Serial.println("{\"status\":\"error\",\"message\":\"Sensor DHT22 tidak terbaca!\"}");
    return;
  }

  // Nyalakan LED indikator pengiriman data
  digitalWrite(PIN_LED, HIGH);

  // Buat objek JSON Document
  JsonDocument doc;
  doc["device_id"] = "ESP32_UNIB_IOT";
  doc["suhu"] = serialized(String(suhu, 2));
  doc["kelembaban"] = serialized(String(kelembaban, 2));
  doc["mode"] = isEmergencyMode ? "DARURAT" : "NORMAL";
  doc["interval_ms"] = intervalTelemetri;
  doc["uptime_ms"] = millis();

  // Serialisasi data JSON ke Serial Monitor
  serializeJson(doc, Serial);
  Serial.println(); // Baris baru agar output rapi

  // Matikan kembali LED indikator
  digitalWrite(PIN_LED, LOW);
}

void setup() {
  Serial.begin(115200);
  
  // Konfigurasi pin I/O
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  digitalWrite(PIN_LED, LOW);

  // Inisialisasi sensor DHT
  dht.begin();

  Serial.println("==================================================");
  Serial.println(" Praktikum Minggu 1 - Teknologi IoT FT UNIB");
  Serial.println(" Sistem Telemetri ESP32 + DHT22 (ArduinoJson)");
  Serial.println(" Mode Awal: NORMAL (Interval 2000 ms)");
  Serial.println(" Tekan Tombol Darurat untuk Ubah Mode ke 500 ms");
  Serial.println("==================================================");
}

void loop() {
  // 1. Tangani input tombol secara non-blocking setiap saat
  handleEmergencyButton();

  // 2. Kirim telemetri periodik secara non-blocking menggunakan millis()
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= intervalTelemetri) {
    previousMillis = currentMillis;
    sendTelemetry();
  }
}
