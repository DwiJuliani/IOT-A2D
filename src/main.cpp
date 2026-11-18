/**
 * ==============================================================================
 * PRAKTIKUM INTERNET OF THINGS (IoT) - MINGGU 02
 * Topik    : GPIO, ADC, PWM, dan Sensor Kelembaban Tanah
 * Modul    : Lab-1 ESP32 (Sesuai Ceklis Verifikasi 3.3)
 * Penulis  : Tim Praktikum IoT A2D
 * ==============================================================================
 * 
 * PEMETAAN PIN AMAN (Item 1 Ceklis):
 * 1. GPIO 34 (ADC1_CH6) : Input Sensor Kelembaban Tanah (Potensiometer) - Input Only, Aman Wi-Fi
 * 2. GPIO 4             : Output Relay Pompa Air (Digital Output)
 * 3. GPIO 15            : Output Modulasi PWM Kipas Pendingin / LED Indikator PWM
 * 4. GPIO 18            : Output Indikator Status & Alarm Dry-Run Safety Cut-Off
 * 5. GPIO 26            : Input Push Button (Internal Pull-Up) untuk Toggle Mode & Reset Safety
 * 
 * FITUR UTAMA:
 * - Filter Moving Average untuk menghaluskan fluktuasi noise ADC (Item 2).
 * - Pemetaan terbalik (Inverse Mapping) 0% (kering/ADC tinggi) s.d. 100% (basah/ADC rendah) (Item 2).
 * - Demonstrasi Perbandingan Relay: Mode Chattering vs Mode Histeresis (Item 3).
 * - Modulasi Kecerahan / Kecepatan Kipas menggunakan sinyal PWM (LEDC ESP32) (Item 3).
 * - Fitur Tambahan C6: Dry-Run Safety Cut-Off dengan Lockout & Manual Reset (Item 4).
 * ==============================================================================
 */

#include <Arduino.h>

// ======================== DEFINISI PIN HARDWARE ========================
const int PIN_SOIL_ADC   = 34; // GPIO 34: ADC1_CH6 (Hanya input, aman dari konflik ADC2/WiFi)
const int PIN_RELAY      = 4;  // GPIO 4 : Output kendali modul relai
const int PIN_PWM_FAN    = 15; // GPIO 15: Output PWM kipas pendingin / LED indikator
const int PIN_LED_ALARM  = 18; // GPIO 18: Indikator peringatan Dry-Run Safety Cut-Off
const int PIN_BTN_MODE   = 26; // GPIO 26: Push-Button (Toggle Mode Chattering/Histeresis & Reset)

// ======================== KONFIGURASI KALIBRASI ADC =====================
// Nilai ADC 12-bit ESP32 (0 - 4095).
// Sensor tanah kapasitif/potensiometer:
// Nilai Tinggi = Tanah Kering (Udara bebas / 0% kelembaban)
// Nilai Rendah = Tanah Basah (Tercelup air / 100% kelembaban)
const int ADC_DRY_CALIBRATION = 3500; // Kalibrasi kondisi kering (0%)
const int ADC_WET_CALIBRATION = 1400; // Kalibrasi kondisi basah (100%)

// ======================== KONFIGURASI FILTER MOVING AVERAGE =============
const int FILTER_WINDOW_SIZE = 10;     // Ukuran jendela rata-rata bergerak
int adcReadings[FILTER_WINDOW_SIZE];   // Array penyimpan riwayat pembacaan
int filterIndex = 0;                   // Indeks lingkaran filter
long filterTotal = 0;                  // Total berjalan pembacaan
bool filterInitialized = false;

// ======================== KONFIGURASI PWM (LEDC) ========================
const int PWM_CHANNEL    = 0;
const int PWM_FREQ       = 5000;       // Frekuensi 5 kHz
const int PWM_RESOLUTION = 8;          // Resolusi 8-bit (0 - 255)

// ======================== PARAMETER KONTROL RELAY =======================
// Ambang Batas untuk Mode Chattering (Ambang tunggal / Single Threshold)
const int CHATTER_THRESHOLD = 40;      // Di bawah 40% pompa ON, di atas 40% OFF

// Ambang Batas untuk Mode Histeresis (Ambang ganda / Dual Threshold)
const int HYST_LOWER_THRESH = 35;      // Batas Bawah: < 35% -> Pompa dinyalakan (ON)
const int HYST_UPPER_THRESH = 55;      // Batas Atas : > 55% -> Pompa dimatikan (OFF)

// ======================== PARAMETER FITUR C6: DRY-RUN SAFETY =============
// Jika pompa ON selama durasi tertentu namun kelembaban TIDAK naik >= kenaikan minimal,
// sistem menyimpulkan air tandon habis (dry run) dan memutus daya relai.
const unsigned long MAX_PUMP_RUN_TIME = 10000; // Maksimal pompa nyala terus menerus: 10 detik
const int MIN_MOISTURE_RISE = 3;               // Minimal kenaikan kelembaban (+3%)

// ======================== VARIABEL STATUS SISTEM ========================
bool isHysteresisMode = true;   // true = Histeresis (aman), false = Chattering (demonstrasi)
bool relayActive = false;        // Status saat ini dari relay pompa
bool dryRunTripped = false;      // Status penguncian darurat Dry-Run Cut-Off

unsigned long pumpStartTime = 0;       // Waktu saat pompa mulai ON
int moistureAtPumpStart = 0;           // Nilai kelembaban awal saat pompa mulai ON
unsigned long lastAlarmBlinkTime = 0;  // Pewaktu kedip LED alarm
bool alarmLedState = false;

// Variabel Debounce Push-Button
int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;

// Pewaktu pengiriman data Serial Telemetri
unsigned long lastTelemetryTime = 0;
const unsigned long TELEMETRY_INTERVAL = 500; // Kirim data setiap 500 ms

// ==============================================================================
// FUNGSI INISIALISASI DAN BANTUAN
// ==============================================================================

/**
 * Filter Moving Average: Mengambil sampel baru dan mengembalikan nilai rata-rata
 */
int updateMovingAverage(int rawSample) {
  if (!filterInitialized) {
    for (int i = 0; i < FILTER_WINDOW_SIZE; i++) {
      adcReadings[i] = rawSample;
    }
    filterTotal = (long)rawSample * FILTER_WINDOW_SIZE;
    filterInitialized = true;
    return rawSample;
  }

  filterTotal = filterTotal - adcReadings[filterIndex];
  adcReadings[filterIndex] = rawSample;
  filterTotal = filterTotal + adcReadings[filterIndex];
  filterIndex = (filterIndex + 1) % FILTER_WINDOW_SIZE;

  return (int)(filterTotal / FILTER_WINDOW_SIZE);
}

/**
 * Pemetaan Terbalik: Menghitung persentase kelembaban (0% - 100%)
 * Nilai ADC tinggi = kering (0%), Nilai ADC rendah = basah (100%)
 */
int calculateMoisturePercent(int adcValue) {
  int percent = map(adcValue, ADC_DRY_CALIBRATION, ADC_WET_CALIBRATION, 0, 100);
  return constrain(percent, 0, 100);
}

/**
 * Konfigurasi PWM Kipas (Dukungan kompatibilitas ESP32 Core 2.x & 3.x)
 */
void setupPWM() {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
  ledcAttach(PIN_PWM_FAN, PWM_FREQ, PWM_RESOLUTION);
#else
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(PIN_PWM_FAN, PWM_CHANNEL);
#endif
}

/**
 * Menulis Nilai PWM (Duty Cycle 0 - 255)
 */
void setFanPWM(int duty) {
  duty = constrain(duty, 0, 255);
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
  ledcWrite(PIN_PWM_FAN, duty);
#else
  ledcWrite(PWM_CHANNEL, duty);
#endif
}

/**
 * Mereset status Dry-Run Safety Cut-Off
 */
void resetDryRunSafety() {
  if (dryRunTripped) {
    dryRunTripped = false;
    digitalWrite(PIN_LED_ALARM, LOW);
    Serial.println(F("\n>>> [RESET] Fitur C6 Dry-Run Safety Cut-Off telah DI-RESET! Sistem kembali normal. <<<\n"));
  }
}

/**
 * Membaca tombol GPIO 26 dengan proteksi debounce
 */
void handleButton() {
  int reading = digitalRead(PIN_BTN_MODE);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    static int buttonState = HIGH;
    if (reading != buttonState) {
      buttonState = reading;
      // Tombol ditekan (Active LOW)
      if (buttonState == LOW) {
        if (dryRunTripped) {
          // Jika sistem sedang terkunci cut-off, tombol berfungsi sebagai RESET
          resetDryRunSafety();
        } else {
          // Jika sistem normal, toggle mode Chattering vs Histeresis
          isHysteresisMode = !isHysteresisMode;
          Serial.print(F("\n>>> [MODE DIUBAH] Mode Kontrol Saat Ini: "));
          Serial.println(isHysteresisMode ? F("HISTERESIS (Aman & Stabil)") : F("CHATTERING (Demonstrasi Getar Relai)"));
        }
      }
    }
  }
  lastButtonState = reading;
}

/**
 * Menangani perintah Serial (opsional untuk kemudahan pengujian praktikan)
 */
void handleSerialCommands() {
  if (Serial.available() > 0) {
    char cmd = (char)Serial.read();
    if (cmd == 'm' || cmd == 'M') {
      isHysteresisMode = !isHysteresisMode;
      Serial.print(F("--> Mode beralih ke: "));
      Serial.println(isHysteresisMode ? F("HISTERESIS") : F("CHATTERING"));
    } else if (cmd == 'r' || cmd == 'R') {
      resetDryRunSafety();
    }
  }
}

// ==============================================================================
// SETUP SISTEM
// ==============================================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println(F("================================================================="));
  Serial.println(F("  PRAKTIKUM MINGGU 02: GPIO, ADC, PWM, DAN SENSOR (LAB-1 ESP32) "));
  Serial.println(F("================================================================="));
  Serial.println(F("[1] Konfigurasi Pin Aman:"));
  Serial.println(F("    - GPIO 34 : ADC1 Input Sensor Tanah (Potensiometer)"));
  Serial.println(F("    - GPIO 4  : Digital Output Relay Pompa Air"));
  Serial.println(F("    - GPIO 15 : Output Modulasi PWM Kipas/LED"));
  Serial.println(F("    - GPIO 18 : Output Indikator Alarm Dry-Run"));
  Serial.println(F("    - GPIO 26 : Input Tombol Toggle Mode & Reset Safety (Pull-Up)"));

  // Konfigurasi Pin
  pinMode(PIN_SOIL_ADC, INPUT);
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_LED_ALARM, OUTPUT);
  pinMode(PIN_BTN_MODE, INPUT_PULLUP);

  // Status awal aktuator
  digitalWrite(PIN_RELAY, LOW);
  digitalWrite(PIN_LED_ALARM, LOW);

  // Setup PWM
  setupPWM();
  setFanPWM(0);

  // Inisialisasi awal pembacaan ADC untuk filter
  int initialSample = analogRead(PIN_SOIL_ADC);
  updateMovingAverage(initialSample);

  Serial.println(F("[2] Filter Moving Average & Inverted Mapping Siap."));
  Serial.println(F("[3] Mode Default: HISTERESIS RELAY (35% - 55%)."));
  Serial.println(F("[4] Proteksi C6: Dry-Run Safety Cut-Off Aktif (Maks 10s)."));
  Serial.println(F("-----------------------------------------------------------------"));
  Serial.println(F("Tekan tombol pada GPIO 26 atau ketik 'm' di Serial untuk ubah Mode."));
  Serial.println(F("Ketik 'r' di Serial untuk me-reset Alarm Dry-Run Cut-Off."));
  Serial.println(F("=================================================================\n"));
}

// ==============================================================================
// LOOP UTAMA
// ==============================================================================
void loop() {
  // 1. Tangani input tombol dan perintah serial
  handleButton();
  handleSerialCommands();

  // 2. Pembacaan ADC & Pemrosesan Sinyal (Item 2 Ceklis)
  int rawADC = analogRead(PIN_SOIL_ADC);
  int filteredADC = updateMovingAverage(rawADC);
  int moisturePercent = calculateMoisturePercent(filteredADC);

  // 3. Modulasi PWM Kipas (Item 3 Ceklis)
  // Semakin kering tanah, semakin tinggi kecepatan kipas (atau modulasi proporsional 0-255)
  // Skala: 0% kelembaban -> PWM 255 (Kipas Full), 100% kelembaban -> PWM 30 (Kipas Minimum/Idle)
  int fanDuty = map(100 - moisturePercent, 0, 100, 30, 255);
  setFanPWM(fanDuty);

  // 4. Logika Pengendalian Relay (Chattering vs Histeresis) (Item 3 Ceklis)
  if (!dryRunTripped) {
    if (isHysteresisMode) {
      // === MODE HISTERESIS (SCHMITT TRIGGER LOGIC) ===
      // Ambang ganda: mencegah relai cetak-cetek (chattering)
      if (moisturePercent <= HYST_LOWER_THRESH) {
        if (!relayActive) {
          relayActive = true;
          pumpStartTime = millis();
          moistureAtPumpStart = moisturePercent;
        }
      } else if (moisturePercent >= HYST_UPPER_THRESH) {
        relayActive = false;
      }
      // Jika berada di antara HYST_LOWER_THRESH dan HYST_UPPER_THRESH, relay pertahankan status lama
    } else {
      // === MODE CHATTERING (SINGLE THRESHOLD LOGIC) ===
      // Menggunakan ambang batas tunggal tanpa histeresis
      // Di sekitar 40%, noise analog akan menyebabkan relay bolak-balik ON-OFF sangat cepat!
      if (moisturePercent < CHATTER_THRESHOLD) {
        if (!relayActive) {
          relayActive = true;
          pumpStartTime = millis();
          moistureAtPumpStart = moisturePercent;
        }
      } else {
        relayActive = false;
      }
    }

    // 5. Fitur Tambahan C6: Dry-Run Safety Cut-Off (Item 4 Ceklis)
    if (relayActive) {
      unsigned long runDuration = millis() - pumpStartTime;
      
      // Jika pompa telah menyala melebihi durasi aman maksimal
      if (runDuration >= MAX_PUMP_RUN_TIME) {
        int moistureGain = moisturePercent - moistureAtPumpStart;
        
        // Periksa apakah kelembaban naik secara signifikan
        if (moistureGain < MIN_MOISTURE_RISE) {
          // AIR HABIS / DRY RUN TERDETEKSI!
          relayActive = false;
          dryRunTripped = true; // Kunci sistem (Lockout)
          digitalWrite(PIN_RELAY, LOW);
          
          Serial.println(F("\n*****************************************************************"));
          Serial.println(F(" [PERINGATAN SISTEM] FITUR C6 DRY-RUN SAFETY CUT-OFF TERPICU!"));
          Serial.println(F(" Pompa telah menyala selama 10 detik tanpa kenaikan kelembaban."));
          Serial.println(F(" Kemungkinan: Tandon air kosong atau pipa penyiram terputus."));
          Serial.println(F(" Tindakan: Relai dimatikan demi keselamatan motor pompa."));
          Serial.println(F(" Tekan tombol GPIO 26 atau ketik 'r' di Serial untuk RESET."));
          Serial.println(F("*****************************************************************\n"));
        } else {
          // Kelembaban bertambah dengan baik, perbarui timer dan basis kelembaban
          pumpStartTime = millis();
          moistureAtPumpStart = moisturePercent;
        }
      }
    }

    // Terapkan output ke pin fisik relay
    digitalWrite(PIN_RELAY, relayActive ? HIGH : LOW);
    digitalWrite(PIN_LED_ALARM, LOW);

  } else {
    // KONDISI EMERGENCY / SAFETY TRIP:
    // Pastikan relay MATI dan indikator alarm berkedip cepat
    digitalWrite(PIN_RELAY, LOW);

    if (millis() - lastAlarmBlinkTime >= 150) {
      lastAlarmBlinkTime = millis();
      alarmLedState = !alarmLedState;
      digitalWrite(PIN_LED_ALARM, alarmLedState ? HIGH : LOW);
    }
  }

  // 6. Pengiriman Data Telemetri Serial (Format Rapi untuk Tabel 3.1 & 3.2)
  if (millis() - lastTelemetryTime >= TELEMETRY_INTERVAL) {
    lastTelemetryTime = millis();

    Serial.print(F("[DATA] Raw ADC: "));
    Serial.print(rawADC);
    Serial.print(F(" | Filtered: "));
    Serial.print(filteredADC);
    Serial.print(F(" | Moisture: "));
    Serial.print(moisturePercent);
    Serial.print(F("% | Fan PWM: "));
    Serial.print(fanDuty);
    Serial.print(F(" ("));
    Serial.print((fanDuty * 100) / 255);
    Serial.print(F("%) | Relay: "));
    Serial.print(relayActive ? F("ON ") : F("OFF"));
    Serial.print(F(" | Mode: "));
    Serial.print(isHysteresisMode ? F("HISTERESIS") : F("CHATTERING"));
    
    if (dryRunTripped) {
      Serial.print(F(" | [STATUS: DRY-RUN LOCKED]"));
    }
    Serial.println();
  }

  delay(20); // Delay siklus eksekusi kecil
}
