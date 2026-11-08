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

// --- Konfigurasi Pin Hardware ---
#define PIN_SOUND_DO       27   // Digital Output sensor suara
#define PIN_SOUND_AO       34   // Analog Output sensor suara
#define PIN_DHT            15   // Pin data DHT22 (Ceklis 2)
#define DHTTYPE            DHT22
#define PIN_LAMP           26   // LED Lampu Utama (Kuning)
#define PIN_STATUS         25   // LED Indikator Tepukan (Hijau)
#define PIN_BUTTON         14   // Tombol Manual / Mode Darurat (Ceklis 2 & C6)
#define PIN_CLAP_SIM       13   // Tombol Simulasi Tepukan Wokwi (CLAP)

// --- Parameter Waktu ---
#define CLAP_DEBOUNCE_MS   120  // Anti-bouncing tepukan
#define CLAP_WINDOW_MS     1000 // Batas waktu tepukan ke-2
#define STATUS_BLINK_MS    100  // Lama kedip LED status
#define INTERVAL_NORMAL    2000 // Mode Normal: 2000 ms
#define INTERVAL_CEPAT     500  // Mode Cepat: 500 ms
#define BUTTON_DEBOUNCE    50   // Debounce tombol manual

DHT dht(PIN_DHT, DHTTYPE);

bool lampOn = false;
int clapCount = 0;
unsigned long firstClapTime = 0;
unsigned long lastClapTime = 0;
unsigned long statusOffTime = 0;
bool lastSoundState = LOW;

bool isEmergencyMode = false;
int lastButtonState = HIGH;
unsigned long lastButtonTime = 0;

unsigned long previousTelemetryMillis = 0;
unsigned long telemetryInterval = INTERVAL_NORMAL;

void setLamp(bool state, const char *triggerSource) {
  lampOn = state;
  digitalWrite(PIN_LAMP, lampOn ? HIGH : LOW);

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

void handleSoundSensor() {
  unsigned long now = millis();
  bool soundDetected = (digitalRead(PIN_SOUND_DO) == HIGH) ||
                       (digitalRead(PIN_CLAP_SIM) == LOW);

  if (soundDetected && !lastSoundState && (now - lastClapTime > CLAP_DEBOUNCE_MS)) {
    lastClapTime = now;
    flashStatus();

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
      setLamp(!lampOn, "double_clap");
      clapCount = 0;
    }
  }
  lastSoundState = soundDetected;

  if (clapCount == 1 && (now - firstClapTime > CLAP_WINDOW_MS)) {
    clapCount = 0;
  }
}

void handleButton() {
  int reading = digitalRead(PIN_BUTTON);

  if (reading == LOW && lastButtonState == HIGH &&
      (millis() - lastButtonTime > BUTTON_DEBOUNCE)) {
    lastButtonTime = millis();

    // Toggle mode interval (Soal C6)
    isEmergencyMode = !isEmergencyMode;
    telemetryInterval = isEmergencyMode ? INTERVAL_CEPAT : INTERVAL_NORMAL;

    // Toggle lampu sebagai saklar manual
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
  Serial.println(" Tepuk tangan 2x (atau tekan tombol CLAP merah) untuk saklar lampu.");
  Serial.println(" Tekan tombol biru untuk saklar manual & ubah interval (C6).");
  Serial.println(" Output Telemetri: JSON Valid (ArduinoJson)");
  Serial.println("==========================================================");
}

void loop() {
  handleSoundSensor();
  handleButton();

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
