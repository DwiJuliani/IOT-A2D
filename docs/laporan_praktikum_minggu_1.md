# Laporan & Dokumentasi Praktikum Minggu 1
## Perancangan Sistem Smart Lamp Berbasis ESP32 Menggunakan Sensor Suara & Pemenuhan Verifikasi Toolchain

**Program Studi:** S1 Informatika, Fakultas Teknik, Universitas Bengkulu  
**Laboratorium:** Jaringan Komputer & Sistem Tertanam FT UNIB  

---

## 👥 Data Kelompok
| No | Nama Mahasiswa | NIM | Peran / Tugas |
|:---:|---|:---:|---|
| 1 | **Dwi Juliani** | G1A024084 | Inisialisasi Repositori, Konfigurasi PlatformIO & Build System |
| 2 | **Adiva Luthfiyyah Yulian** | G1A024110 | Firmware Logic (Non-blocking, Double Clap & Mode Interval C6) |
| 3 | **Agief Vemas A** | G1A024037 | Wiring Diagram Wokwi (Sensor Suara + DHT22), Payload JSON & Testing |

---

## 📋 Tabel Pemenuhan Ceklis Verifikasi 3.3

### 1. Toolchain & Zero Error Compilation (Komponen 1)
* Menggunakan **PlatformIO IDE** (`platformio.ini`) berbasis framework Arduino untuk mikrokontroler `esp32dev`.
* Dependensi pustaka:
  * `bblanchon/ArduinoJson @ ^7.0.4`
  * `adafruit/DHT sensor library @ ^1.4.6`
  * `adafruit/Adafruit Unified Sensor @ ^1.1.14`
* Hasil kompilasi: `.pio/build/esp32dev/firmware.bin` berhasil dikompilasi tanpa galat (*zero error*).

### 2. Simulasi Wokwi: ESP32 + DHT22 + LED + Pushbutton + Sensor Suara (Komponen 2)
* Seluruh komponen yang disyaratkan (ESP32, DHT22, LED, Pushbutton) terintegrasi secara harmonis dengan fitur utama Smart Lamp (Sensor Suara KY-038).
* Tautan Simulasi: 👉 **[Wokwi Project 479765417567553537](https://wokwi.com/projects/479765417567553537)**
* **Tabel Pinout:**
  | Komponen | Pin Komponen | Pin ESP32 DevKit | Fungsi / Keterangan |
  |---|---|---|---|
  | **Sensor Suara KY-038** | VCC | 3V3 | Catu daya |
  | | GND | GND | Ground |
  | | DO | GPIO 27 | Deteksi digital tepukan |
  | | AO | GPIO 34 | Level kebisingan analog |
  | **Sensor DHT22** | VCC | 3V3 | Catu daya |
  | | GND | GND | Ground |
  | | SDA | GPIO 15 | Input suhu & kelembaban (Ceklis 2) |
  | **LED Lampu Utama** | Anoda (via R 220 Ω) | GPIO 26 | Lampu pintar (Kuning) |
  | | Katoda | GND | Ground |
  | **LED Status Tepukan** | Anoda (via R 220 Ω) | GPIO 25 | Indikator deteksi suara (Hijau) |
  | | Katoda | GND | Ground |
  | **Pushbutton Biru** | Kaki 1 / 2 | GPIO 14 / GND | Saklar manual & Pengubah interval telemetri (C6) |
  | **Pushbutton Merah** | Kaki 1 / 2 | GPIO 13 / GND | Simulasi suara tepukan tangan (CLAP) di Wokwi |

* **Contoh Keluaran JSON Valid di Serial Monitor:**
```json
{"system":"SmartLamp_ESP32","lamp_status":"ON","suhu":28.50,"kelembaban":65.20,"sound_level":2048,"mode":"NORMAL","interval_ms":2000,"uptime_ms":8000}
```

---

## ⏱️ Demonstrasi: Mode Blocking vs Non-Blocking (Komponen 3)

### A. Kelemahan Mode Blocking (Fungsi `delay()`)
```cpp
// CONTOH BLOCKING:
void loop() {
  sendPeriodicTelemetry();
  delay(2000); // CPU berhenti total selama 2 detik!
  // Jika tepukan tangan atau penekanan tombol terjadi saat delay, 
  // mikrofon/tombol TIDAK AKAN TERDETEKSI (missed trigger).
}
```

### B. Keunggulan Mode Non-Blocking (Fungsi `millis()`)
```cpp
// IMPLEMENTASI DI src/main.cpp:
void loop() {
  handleSoundSensor(); // Dicek setiap mikrodetik secara instan
  handleButton();      // Dicek setiap mikrodetik secara instan

  unsigned long currentMillis = millis();
  if (currentMillis - previousTelemetryMillis >= telemetryInterval) {
    previousTelemetryMillis = currentMillis;
    sendPeriodicTelemetry();
  }
}
```
* **Hasil:** Mikrokontroler dapat menjalankan fungsi sensor suara (double clap) dan penekanan tombol darurat seketika tanpa ada penundaan, sekaligus tetap mengirimkan data telemetri berkala secara teratur.

---

## 📝 Jawaban Tugas Evaluasi [C5 - Mengevaluasi]

> **Soal:** Bandingkan kelebihan dan kekurangan membangun payload JSON menggunakan pustaka **ArduinoJson (`JsonDocument`)** dibanding merangkai string secara manual dengan **`Serial.print` / `sprintf`** ditinjau dari keamanan memori dan kemudahan pemeliharaan kode!

| Parameter Evaluasi | Pustaka ArduinoJson (`JsonDocument`) | Manual String (`Serial.print` / `sprintf`) |
|---|---|---|
| **Keamanan Memori (*Memory Safety*)** | **Sangat Aman & Terkelola:**<br>1. Alokasi memori dinamis dikelola secara otomatis dan dibersihkan saat keluar scope (RAII pattern).<br>2. Tidak memerlukan deklarasi buffer statis manual berisiko.<br>3. Mencegah **Buffer Overflow** dan **Heap Fragmentation**. | **Rentan Galat Memori:**<br>1. Penggunaan `sprintf` dengan `char buffer[128]` rentan *buffer overrun/stack corruption* jika data sensor melebihi kapasitas.<br>2. Penggabungan string dengan objek `String` Arduino rentan menyebabkan fragmentasi memori RAM. |
| **Kemudahan Pemeliharaan (*Maintainability*)** | **Sangat Baik & Modular:**<br>1. Sintaks *key-value* intuitif (`doc["suhu"] = 28.5`).<br>2. Otomatis menangani *escaping character* (kutip ganda `\"`).<br>3. Penambahan atribut baru tidak merusak format. | **Rentan Salah Sintaks:**<br>1. Format manual mudah salah letak koma, kurung kurawal, atau tanda kutip ganda.<br>2. Sulit dibaca dan dirawat ketika payload memiliki banyak data. |
| **Konsumsi Resource** | Memerlukan ukuran Flash sedikit lebih besar untuk parser library (~3 KB). | Ukuran biner Flash sedikit lebih kecil untuk format sangat sederhana. |

---

## 💡 Jawaban Tugas [C6 - Mencipta]

> **Soal:** Modifikasilah kode `main.cpp` pada Eksperimen 3 agar ketika tombol darurat ditekan, interval pengiriman telemetri otomatis berubah dari **2000 ms (mode normal)** menjadi **500 ms (mode cepat)**, dan kembali ke **2000 ms** jika tombol ditekan sekali lagi!

* **Implementasi:**
  1. Tombol manual di pin GPIO 14 difungsikan ganda sebagai saklar lampu cadangan dan *toggle* mode telemetri darurat.
  2. Saat tombol ditekan, status `isEmergencyMode` berbalik (*toggle*):
     * Jika `isEmergencyMode == true`: `telemetryInterval = 500` ms (Mode Darurat/Cepat).
     * Jika `isEmergencyMode == false`: `telemetryInterval = 2000` ms (Mode Normal).
  3. Dilengkapi algoritma **Debounce 50 ms** untuk mencegah pemicuan ganda akibat getaran mekanik saklar.
  4. Status `mode` dan nilai `interval_ms` dikirimkan secara transparan di dalam payload JSON telemetri.
