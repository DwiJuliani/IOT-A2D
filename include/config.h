#ifndef CONFIG_H
#define CONFIG_H

// ==========================================
// Konfigurasi Pin Hardware (ESP32 DevKit v4)
// ==========================================
#define PIN_DHT          15    // Pin Data Sensor Suhu & Kelembaban DHT22
#define PIN_LED          2     // Pin LED Indikator Telemetri / Alarm
#define PIN_BUTTON       14    // Pin Tombol Darurat (Mode Non-Blocking INPUT_PULLUP)

// ==========================================
// Konfigurasi Sensor & Waktu
// ==========================================
#define DHTTYPE          DHT22 // Sensor DHT22 (AM2302)

// Interval Telemetri (ms) - Sesuai Soal C6
#define INTERVAL_NORMAL  2000  // Mode Normal: 2000 ms
#define INTERVAL_CEPAT   500   // Mode Darurat/Cepat: 500 ms
#define DEBOUNCE_DELAY   50    // Anti-bouncing tombol (ms)

#endif // CONFIG_H
