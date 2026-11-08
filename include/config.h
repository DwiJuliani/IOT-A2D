#ifndef CONFIG_H
#define CONFIG_H

// ====================================================================
// Konfigurasi Pin Perancangan Sistem Smart Lamp Berbasis ESP32
// Menggunakan Sensor Suara & Sensor DHT22 (Sesuai Ceklis Praktikum)
// ====================================================================

// --- Sensor Suara (KY-038) ---
#define PIN_SOUND_DO       27   // Digital Output sensor suara (deteksi tepukan)
#define PIN_SOUND_AO       34   // Analog Output sensor suara (tingkat kebisingan)

// --- Sensor Suhu & Kelembaban (DHT22) - Ceklis 2 ---
#define PIN_DHT            15   // Pin data DHT22
#define DHTTYPE            DHT22

// --- Aktuator & Indikator LED ---
#define PIN_LAMP           26   // LED Lampu Utama (Kuning)
#define PIN_STATUS         25   // LED Indikator Tepukan / Status (Hijau)

// --- Tombol Input (Non-Blocking) ---
#define PIN_BUTTON         14   // Tombol Manual / Mode Darurat (INPUT_PULLUP)
#define PIN_CLAP_SIM       13   // Tombol Simulasi Tepukan Wokwi (INPUT_PULLUP)

// --- Parameter Waktu (Non-Blocking millis()) ---
#define CLAP_DEBOUNCE_MS   120  // Jeda minimum antar tepukan (anti-bouncing)
#define CLAP_WINDOW_MS     1000 // Jendela waktu tepukan ke-2 (maksimal 1 detik)
#define STATUS_BLINK_MS    100  // Durasi kedip LED indikator status

// Interval Pengiriman Telemetri JSON (Sesuai Soal C6)
#define INTERVAL_NORMAL    2000 // Mode Normal: 2000 ms (2 detik)
#define INTERVAL_CEPAT     500  // Mode Cepat / Darurat: 500 ms (0.5 detik)
#define BUTTON_DEBOUNCE    50   // Debounce tombol manual (ms)

#endif // CONFIG_H
