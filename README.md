# Perancangan Sistem Smart Lamp Berbasis ESP32 Menggunakan Sensor Suara
**Program Studi S1 Informatika, Fakultas Teknik, Universitas Bengkulu**  
**Mata Kuliah:** Teknologi IoT | Praktikum Minggu 1 (Lab-0: Toolchain & Wokwi Simulator)

> 🔗 **Tautan Simulasi Wokwi:** [https://wokwi.com/projects/480138974654676993](https://wokwi.com/projects/480138974654676993)

---

## 1. Deskripsi Proyek
Proyek ini merancang sistem lampu pintar (**Smart Lamp**) yang dikendalikan dengan **tepukan tangan dua kali (double clap)** menggunakan ESP32 dan sensor suara KY-038. Sistem ini juga mengintegrasikan sensor suhu & kelembaban **DHT22**, indikator LED, dan tombol manual/darurat untuk memenuhi seluruh kriteria verifikasi unjuk kerja praktikum IoT (Tabel 3.3 dan Tugas C5 & C6).

---

## 2. Anggota Kelompok
| No | Nama Lengkap | NIM | Peran Utama |
|:---:|---|:---:|---|
| 1 | **Dwi Juliani** | G1A024084 | Setup PlatformIO, Toolchain & Dependencies |
| 2 | **Adiva Luthfiyyah Yulian** | G1A024110 | Firmware Logic (Non-blocking, Double Clap & C6) |
| 3 | **Agief Vemas A** | G1A024037 | Wiring Diagram Wokwi, Telemetri JSON & Testing |

---

## 3. Struktur 5 Folder Standar Repositori
Repositori ini disusun mengikuti arsitektur proyek PlatformIO / IoT Lab standar:
```text
IOT-A2D/
├── src/                  # Folder 1: Berkas sumber utama (main.cpp)
├── include/              # Folder 2: File header konfigurasi hardware (config.h)
├── lib/                  # Folder 3: Modul / pustaka lokal
├── docs/                 # Folder 4: Laporan, evaluasi C5, dokumentasi blocking vs non-blocking
├── diagram/              # Folder 5: Skematik rangkaian Wokwi (diagram.json)
├── platformio.ini        # File konfigurasi PlatformIO Build System
├── diagram.json          # File rangkaian Wokwi Simulator
├── wokwi.toml            # File integrasi Wokwi di VS Code
└── README.md             # Dokumentasi utama proyek
```

---

## 4. Wiring Hardware & Komponen

| Komponen | Pin Komponen | Pin ESP32 DevKit | Fungsi |
|---|---|---|---|
| **Sensor Suara KY-038** | VCC | 3V3 | Catu daya |
| | GND | GND | Ground |
| | DO | GPIO 27 | Deteksi tepukan suara |
| | AO | GPIO 34 | Level kebisingan analog |
| **Sensor DHT22** | VCC | 3V3 | Catu daya |
| | GND | GND | Ground |
| | SDA | GPIO 15 | Input suhu & kelembaban (Ceklis 2) |
| **LED Lampu Utama** | Anoda (via R 220 Ω) | GPIO 26 | Lampu pintar (Kuning) |
| | Katoda | GND | Ground |
| **LED Status Tepukan** | Anoda (via R 220 Ω) | GPIO 25 | Indikator tepukan suara (Hijau) |
| | Katoda | GND | Ground |
| **Pushbutton Biru** | Kaki 1 / 2 | GPIO 14 / GND | Saklar manual & Mode Cepat (C6) |
| **Pushbutton Merah** | Kaki 1 / 2 | GPIO 13 / GND | Simulasi tepukan tangan (CLAP) di Wokwi |

---

## 5. Cara Kerja Sistem
1. **Deteksi Double Clap:**
   - Ketika tepukan pertama terdeteksi, LED hijau berkedip singkat dan mikrokontroler membuka jendela waktu 1 detik.
   - Jika tepukan kedua terjadi dalam rentang waktu < 1 detik, status lampu utama (LED kuning) di-*toggle* (ON $\leftrightarrow$ OFF).
2. **Telemetri JSON Valid (ArduinoJson):**
   - ESP32 secara periodik mengirimkan data telemetri berformat JSON ke Serial Monitor yang mencakup status lampu, suhu, kelembaban, level suara, mode, dan interval.
3. **[C6 - Mencipta] Kontrol Interval Mode Darurat:**
   - Mode Normal: Telemetri dikirim setiap **2000 ms**.
   - Mode Cepat/Darurat: Ketika tombol biru ditekan, interval otomatis berubah menjadi **500 ms** (dan lampu di-*toggle* secara manual).
   - Menekan tombol sekali lagi mengembalikan interval ke **2000 ms**.
4. **Non-Blocking Execution (`millis()`):**
   - Tidak ada fungsi `delay()` yang menghalangi CPU, sehingga deteksi tepukan suara dan tombol manual tetap responsif 100% setiap saat.

---

## 6. Panduan Menjalankan Simulasi di Wokwi
1. Buka tautan simulasi: 👉 [Wokwi Project 480138974654676993](https://wokwi.com/projects/480138974654676993).
2. Salin isi file `sketch.ino` dan `diagram.json` ke proyek Wokwi.
3. Klik tombol **Start Simulation**.
4. Buka tab **Serial Monitor** (115200 baud).
5. Klik tombol merah **CLAP** 2 kali berturut-turut untuk menyalakan/mematikan lampu.
6. Klik tombol biru **Manual / Mode** untuk menyalakan lampu manual dan mengubah interval pengiriman telemetri dari 2000 ms ke 500 ms.

---

## 7. Pemenuhan Ceklis Verifikasi Laboratorium (Tabel 3.3)
1. **Toolchain PlatformIO:** `platformio.ini` terkonfigurasi dengan pustaka `ArduinoJson` & `DHT sensor library` (`firmware.bin zero error`).
2. **ESP32 + DHT22 + LED + Pushbutton:** Terintegrasi di simulasi Wokwi dengan output JSON valid.
3. **Demonstrasi Blocking vs Non-Blocking:** Terdokumentasi lengkap di [`docs/laporan_praktikum_minggu_1.md`](file:///c:/Users/LENOVO/Downloads/IOT-A2D/docs/laporan_praktikum_minggu_1.md).
4. **Struktur 5 Folder & Riwayat Commit Tim:** Repositori memiliki 5 folder standar dengan commit dari seluruh anggota tim (Dwi Juliani, Adiva Luthfiyyah Yulian, Agief Vemas A).
