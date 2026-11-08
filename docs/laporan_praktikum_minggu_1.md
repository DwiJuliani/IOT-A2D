# Laporan & Dokumentasi Praktikum Minggu 1: Toolchain & Wokwi Simulator
**Mata Kuliah:** Teknologi IoT  
**Program Studi:** S1 Informatika, Fakultas Teknik, Universitas Bengkulu  
**Laboratorium:** Jaringan Komputer & Sistem Tertanam FT UNIB  

---

## 👥 Data Kelompok
| No | Nama Mahasiswa | NIM | Peran / Tugas |
|:---:|---|:---:|---|
| 1 | **Dwi Juliani** | G1A024084 | Inisialisasi Repositori, Konfigurasi PlatformIO & Dependencies |
| 2 | **Adiva Luthfiyyah Yulian** | G1A024110 | Desain Firmware, Logika Non-Blocking & Tombol Darurat C6 |
| 3 | **Agief Vemas A** | G1A024037 | Rangkaian Skematik Wokwi, JSON Payload & Pengujian Telemetri |

---

## 📋 Pemenuhan Ceklis Verifikasi 3.3

### 1. Toolchain & Zero Error Compilation (Komponen 1)
- Proyek dikonfigurasi menggunakan **PlatformIO IDE** (`platformio.ini`) berbasis framework Arduino untuk mikrokontroler `esp32dev`.
- Library yang digunakan:
  - `bblanchon/ArduinoJson @ ^7.0.4`
  - `adafruit/DHT sensor library @ ^1.4.6`
  - `adafruit/Adafruit Unified Sensor @ ^1.1.14`
- Hasil kompilasi menghasilkan file binary firmware: `.pio/build/esp32dev/firmware.bin` tanpa galat (*zero error*).

### 2. Simulasi Wokwi: ESP32 + DHT22 + LED + Pushbutton (Komponen 2)
- Rangkaian disimulasikan menggunakan Wokwi dengan tautan:
  👉 **[Simulasi Wokwi Proyek](https://wokwi.com/projects/479765417567553537)**
- Tabel Wiring Pinout:
  | Komponen | Pin Komponen | Pin ESP32 DevKit | Keterangan |
  |---|---|---|---|
  | **Sensor DHT22** | VCC | 3V3 | Catu daya sensor |
  | | GND | GND | Ground |
  | | SDA (Data) | GPIO 15 | Input data suhu & kelembaban |
  | **LED Indikator** | Anoda (via R 220 Ω) | GPIO 2 | Indikator pengiriman data |
  | | Katoda | GND | Ground |
  | **Pushbutton** | Kaki 1 | GPIO 14 | Tombol darurat (INPUT_PULLUP) |
  | | Kaki 2 | GND | Ground |

- Contoh Output Serial Monitor (Format JSON Valid):
```json
{"device_id":"ESP32_UNIB_IOT","suhu":28.40,"kelembaban":65.20,"mode":"NORMAL","interval_ms":2000,"uptime_ms":4000}
```

---

## ⏱️ Demonstrasi: Mode Blocking vs Non-Blocking (Komponen 3)

### A. Mode Blocking (Menggunakan `delay()`)
```cpp
// CONTOH KODE BLOCKING (TIDAK DIGUNAKAN DI FINAL CODE)
void loop() {
  sendTelemetry();
  delay(2000); // CPU berhenti selama 2 detik!
  // Jika tombol ditekan saat delay berlangsung, penekanan TIDAK TERDETEKSI.
}
```
* **Kelemahan:** Instruksi `delay(2000)` menghentikan eksekusi thread CPU secara penuh. Ketika tombol ditekan di antara rentang waktu jeda tersebut, mikrokontroler tidak dapat membaca pin GPIO tombol, sehingga terjadi kondisi *unresponsive* / *missed trigger*.

### B. Mode Non-Blocking (Menggunakan `millis()`)
```cpp
// KODE NON-BLOCKING (DIIMPLEMENTASIKAN DI src/main.cpp)
void loop() {
  handleEmergencyButton(); // Dieksekusi ribuan kali per detik tanpa henti

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= intervalTelemetri) {
    previousMillis = currentMillis;
    sendTelemetry();
  }
}
```
* **Keunggulan:** Mikrokontroler terus melakukan perulangan `loop()` secara kontinu. Pembacaan tombol (`handleEmergencyButton`) diperiksa secara instan pada setiap siklus CPU, sehingga sistem selalu responsif mendeteksi interupsi tombol darurat tanpa terpengaruh durasi interval telemetri.

---

## 📝 Jawaban Tugas Evaluasi [C5 - Mengevaluasi]

> **Soal:** Bandingkan kelebihan dan kekurangan membangun payload JSON menggunakan pustaka **ArduinoJson (`JsonDocument`)** dibanding merangkai string secara manual dengan **`Serial.print` / `sprintf`** ditinjau dari keamanan memori dan kemudahan pemeliharaan kode!

| Parameter Evaluasi | Pustaka ArduinoJson (`JsonDocument`) | Manual String (`Serial.print` / `sprintf`) |
|---|---|---|
| **Keamanan Memori (*Memory Safety*)** | **Sangat Aman & Terkelola:**<br>1. Menggunakan memori yang terkelola dengan baik (pada ArduinoJson v7, `JsonDocument` mengalokasikan memori dinamis secara otomatis dan melepaskannya saat keluar dari scope/RAII).<br>2. Mencegah bahaya **Buffer Overflow** karena tidak memerlukan deklarasi ukuran buffer statis manual.<br>3. Mencegah **Heap Fragmentation** dibandingkan penggabungan objek `String` bawaan Arduino (`+` operator). | **Rentan Galat Memori:**<br>1. Fungsi `sprintf` membutuhkan buffer array statis seperti `char buf[128]`. Jika panjang data melebihi ukuran array, akan terjadi **stack overflow / buffer overrun** yang memicu *ESP32 Guru Meditation Error (Core Panic)*.<br>2. Penggunaan `Serial.print` berulang memakan waktu I/O CPU lebih lama dan rentan *race condition* jika terinterupsi. |
| **Kemudahan Pemeliharaan (*Maintainability*)** | **Sangat Baik & Modular:**<br>1. Sintaks berbasis key-value yang deklaratif (`doc["suhu"] = 28.5`).<br>2. Otomatis menangani *escaping character* (misalnya tanda petik ganda `\"` dan karakter khusus).<br>3. Mendukung konversi tipe data otomatis (integer, float, boolean, nested object/array). | **Sulit Dirawat & Rawan Kesalahan Manusia:**<br>1. Sintaks format specifier (`%s`, `%d`, `%.2f`) rentan tidak cocok dengan tipe data variabel (dapat menyebabkan crash).<br>2. Penulisan manual tanda kutip dan kurung kurawal (`{\"temp\":%f}`) sangat membingungkan ketika struktur JSON semakin kompleks. |
| **Penggunaan Sumber Daya Hardware** | Memerlukan ruang Flash memori ekstra untuk mengompilasi kode parser pustaka (~3-5 KB). | Ukuran kode biner Flash lebih kecil untuk format string sederhana. |

**Kesimpulan Evaluasi:** Untuk aplikasi IoT industri dan skala produksi yang memerlukan keandalan tinggi (*robustness*), penggunaan pustaka **ArduinoJson** jauh lebih direkomendasikan karena menjamin keutuhan struktur data dan keamanan alokasi memori ESP32.

---

## 💡 Jawaban Tugas [C6 - Mencipta]

> **Soal:** Modifikasilah kode `main.cpp` pada Eksperimen 3 agar ketika tombol darurat ditekan, interval pengiriman telemetri otomatis berubah dari **2000 ms (mode normal)** menjadi **500 ms (mode cepat)**, dan kembali ke **2000 ms** jika tombol ditekan sekali lagi!

**Logika Implementasi:**
1. Variabel `isEmergencyMode` (boolean) digunakan sebagai penyimpan *state* sistem.
2. Ketika transisi penekanan tombol dideteksi (dari status HIGH ke LOW pada pin dengan `INPUT_PULLUP`), variabel `isEmergencyMode` di-*toggle*:
   ```cpp
   isEmergencyMode = !isEmergencyMode;
   intervalTelemetri = isEmergencyMode ? 500 : 2000;
   ```
3. Ditambahkan algoritma **Software Debounce** (50 ms) untuk mencegah fenomena *bouncing* mekanik pada tombol yang dapat menyebabkan mode berganti berkali-kali dalam satu kali tekan.
4. Nilai `interval_ms` dan nama `mode` disertakan langsung ke dalam payload JSON telemetri untuk verifikasi real-time di Serial Monitor.
