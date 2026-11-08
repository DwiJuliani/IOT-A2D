/**
 * Praktikum Minggu 1 - Teknologi IoT (Lab-0: Toolchain & Wokwi Simulator)
 * Program Studi S1 Informatika, Fakultas Teknik, Universitas Bengkulu
 * 
 * Simulasi Wokwi: ESP32 + DHT22 + LED + Pushbutton (Emergency)
 * Format Output: JSON Valid (ArduinoJson)
 * 
 * Fitur:
 * - Pembacaan Suhu & Kelembaban DHT22
 * - Non-blocking Telemetry menggunakan millis()
 * - [C6 - Mencipta] Tombol darurat mengubah interval:
 *   Mode Normal = 2000 ms, Mode Cepat/Darurat = 500 ms
 */

#include <Arduino.h>
#include <DHT.h>
#include <ArduinoJson.h>

// --- Konfigurasi Pin ---
#define PIN_DHT          15    // Pin Data DHT22
#define DHTTYPE          DHT22 // Sensor DHT22
#define PIN_LED          2     // Pin LED Indikator
#define PIN_BUTTON       14    // Pin Tombol Darurat (INPUT_PULLUP)

// --- Konfigurasi Interval (ms) ---
#define INTERVAL_NORMAL  2000
#define INTERVAL_CEPAT   500
#define DEBOUNCE_DELAY   50

// Inisialisasi Sensor DHT
DHT dht(PIN_DHT, DHTTYPE);

// Variabel Kontrol Waktu & Mode
unsigned long previousMillis = 0;
unsigned long intervalTelemetri = INTERVAL_NORMAL;
bool isEmergencyMode = false;

// Variabel Debouncing Tombol
int buttonState = HIGH;
int lastButtonReading = HIGH;
unsigned long lastDebounceTime = 0;

void handleEmergencyButton() {
  int currentReading = digitalRead(PIN_BUTTON);

  if (currentReading != lastButtonReading) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (currentReading != buttonState) {
      buttonState = currentReading;

      // Terdeteksi penekanan tombol (aktif LOW)
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

void sendTelemetry() {
  float suhu = dht.readTemperature();
  float kelembaban = dht.readHumidity();

  if (isnan(suhu) || isnan(kelembaban)) {
    Serial.println("{\"status\":\"error\",\"message\":\"Gagal membaca sensor DHT22!\"}");
    return;
  }

  // Kedipkan LED indikator pengiriman data
  digitalWrite(PIN_LED, HIGH);

  // Buat payload JSON
  JsonDocument doc;
  doc["device_id"] = "ESP32_UNIB_IOT";
  doc["suhu"] = serialized(String(suhu, 2));
  doc["kelembaban"] = serialized(String(kelembaban, 2));
  doc["mode"] = isEmergencyMode ? "DARURAT" : "NORMAL";
  doc["interval_ms"] = intervalTelemetri;
  doc["uptime_ms"] = millis();

  // Serialisasi data JSON ke Serial Monitor
  serializeJson(doc, Serial);
  Serial.println();

  digitalWrite(PIN_LED, LOW);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  digitalWrite(PIN_LED, LOW);

  dht.begin();

  Serial.println("==================================================");
  Serial.println(" Praktikum Minggu 1 - Teknologi IoT FT UNIB");
  Serial.println(" Simulasi ESP32 + DHT22 + LED + Pushbutton");
  Serial.println(" Mode Awal: NORMAL (Interval 2000 ms)");
  Serial.println(" Tekan Tombol Darurat untuk Ubah Interval ke 500 ms");
  Serial.println("==================================================");
}

void loop() {
  // Tangani tombol secara responsif (non-blocking)
  handleEmergencyButton();

  // Kirim telemetri periodik secara non-blocking
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= intervalTelemetri) {
    previousMillis = currentMillis;
    sendTelemetry();
  }
}
