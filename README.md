# Praktikum IoT Minggu 02: GPIO, ADC, PWM, & Sensor Kelembaban Tanah

Proyek ini dibuat untuk memenuhi seluruh poin penilaian pada **Bagian 3.3 Ceklis Verifikasi Laboratorium & Komponen Penilaian Laporan Lab-1 (Bobot 2% NA)** pada dokumen panduan `Praktikum_Minggu_02_GPIO_ADC_PWM_Sensor.pdf`.

---

## 📁 Struktur Berkas di VS Code

```text
IOT WEEK 2/
├── CEKLIS_VERIFIKASI.md         # Tabel 3.3 Ceklis Verifikasi Lab & Panduan Demo Asisten
├── LAPORAN_PRAKTIKUM_WEEK_2.md  # Draf Laporan Lengkap (Tabel 3.1 & 3.2, Jawaban C1–C6)
├── IOT_Week_2.ino               # Kode Arduino IDE / Arduino CLI untuk ESP32
├── src/
│   └── main.cpp                 # Kode C++ utama PlatformIO untuk ESP32
├── platformio.ini               # Konfigurasi PlatformIO untuk VS Code
├── diagram.json                 # Skema simulasi interaktif Wokwi di VS Code
├── wokwi.toml                   # Konfigurasi simulator Wokwi
└── README.md                    # Petunjuk penggunaan dan verifikasi
```

---

## 📌 Pemetaan Pin ESP32 (Sesuai Poin 1 Ceklis)

| Komponen | Pin ESP32 | Keterangan |
|---|---|---|
| **Sensor Tanah / Potensiometer** | `GPIO 34` | ADC1_CH6 (*Input Only*, aman saat Wi-Fi aktif) |
| **Modul Relay Pompa Air** | `GPIO 4` | Digital Output kendali relai |
| **Kipas / LED Indikator PWM** | `GPIO 15` | Sinyal PWM LEDC (5 kHz, 8-bit resolusi) |
| **LED Alarm Dry-Run Cut-Off** | `GPIO 18` | Indikator visual peringatan air habis |
| **Push-Button Mode & Reset** | `GPIO 26` | Tombol internal *pull-up* untuk ganti mode / reset alarm |

---

## 🚀 Cara Menjalankan & Menguji di VS Code

### Opsi 1: Menggunakan PlatformIO di VS Code
1. Buka folder `IOT WEEK 2` di VS Code.
2. Pasang ekstensi **PlatformIO IDE** (jika belum ada).
3. Sambungkan board ESP32 via kabel USB.
4. Klik tombol **Build (✓)** di status bar bawah VS Code untuk mengompilasi.
5. Klik tombol **Upload (→)** untuk mengunggah program ke ESP32.
6. Buka **Serial Monitor** pada baud rate `115200` untuk melihat data telemetri.

### Opsi 2: Simulasi Langsung di VS Code via Ekstensi Wokwi
1. Pasang ekstensi **Wokwi Simulator** di VS Code.
2. Buka berkas `diagram.json`.
3. Tekan `F1` lalu pilih `Wokwi: Start Simulator`.
4. Anda dapat langsung menggeser potensiometer, melihat relai aktif, melihat modulasi kecerahan LED PWM, dan menekan tombol di GPIO 26!

---

## 🎯 Panduan Demonstrasi ke Asisten Laboratorium

Saat asisten laboratorium melakukan verifikasi di meja Anda:
1. **Verifikasi Pin Aman & Kompilasi (Poin 1):**  
   Tunjukkan kode `src/main.cpp` atau `IOT_Week_2.ino` yang terkompilasi sukses 0 galat (*zero error*) dan jelaskan bahwa GPIO 34 adalah ADC1 yang aman.
2. **Moving Average & Inverse Mapping (Poin 2):**  
   Buka Serial Monitor. Putar potensiometer dari ujung ke ujung. Tunjukkan bahwa nilai `Filtered ADC` stabil tanpa fluktuasi derau, dan nilai kelembaban terpetakan terbalik: `ADC tinggi = 0%`, `ADC rendah = 100%`.
3. **Demonstrasi Chattering vs Histeresis & Modulasi PWM (Poin 3):**  
   - Beralih ke **Mode Chattering** (tekan tombol GPIO 26 atau ketik `m` di Serial Monitor). Posisikan nilai di sekitar $40\%$, tunjukkan relai bergetar/berpindah sangat cepat akibat *noise*.
   - Beralih ke **Mode Histeresis** (tekan tombol GPIO 26 lagi). Tunjukkan pada rentang $35\% - 55\%$ relai stabil dan tidak pernah *chattering*.
   - Tunjukkan modulasi PWM kipas di GPIO 15 yang kecepatannya berubah halus proporsional.
4. **Fitur C6 Dry-Run Safety Cut-Off & Bukti Git (Poin 4):**  
   - Buat kondisi kelembaban $< 35\%$ sehingga relai menyala.
   - Jangan ubah potensiometer selama 10 detik (simulasi tandon air habis).
   - Tunjukkan relai seketika mati paksa (**CUT-OFF**), LED Alarm di GPIO 18 berkedip cepat, dan sistem terkunci.
   - Tekan tombol GPIO 26 untuk mereset.
   - Buka terminal VS Code dan jalankan `git log --oneline` untuk membuktikan kode dan draf laporan sudah di-*commit*.

---

## 💻 Perintah Git untuk Push ke Cabang Kelompok

Jalankan perintah ini di terminal VS Code:
```bash
git add .
git commit -m "feat(lab-1): penyelesaian tugas praktikum week 2 gpio adc pwm sensor dan laporan lengkap"
git push origin kelompok-a2d
```
*(Ganti `kelompok-a2d` dengan nama cabang Git kelompok Anda)*.
