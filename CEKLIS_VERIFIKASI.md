# CEKLIS VERIFIKASI LABORATORIUM & PENILAIAN LAPORAN LAB-1

**Mata Kuliah** : Internet of Things (IoT)  
**Modul**       : Praktikum Minggu 02 - GPIO, ADC, PWM, dan Sensor  
**Target Board**: ESP32 Dev Module / NodeMCU ESP32  
**Bobot Nilai** : 2% Nilai Akhir (NA)  

---

### 3.3 Ceklis Verifikasi Laboratorium & Komponen Penilaian Laporan Lab-1 (Bobot 2% NA)

| No | Unjuk Kerja yang Diverifikasi Asisten Laboratorium | Status | Paraf Asisten |
|:--:|:---|:---:|:---:|
| **1** | Rangkaian terhubung pada pin aman (GPIO 34 ADC1, GPIO 4, 15, 18, 26) dan terkompilasi tanpa galat. | [x] **Lulus** | ____________ |
| **2** | Pembacaan sensor tanah terfilter (*moving average*) dan pemetaan terbalik 0%–100% berjalan akurat sesuai perubahan potensiometer/sensor. | [x] **Lulus** | ____________ |
| **3** | Mahasiswa berhasil mendemonstrasikan perbedaan perilaku *chattering* vs histeresis relai serta modulasi kecerahan PWM kipas. | [x] **Lulus** | ____________ |
| **4** | Fitur tambahan C6 (*Dry-Run Safety Cut-Off*) berfungsi dan kode + draf laporan telah di-*push* ke cabang Git kelompok. | [x] **Lulus** | ____________ |

---

### Komposisi Nilai Lab-1 (Sesuai RPS Bagian 5.5)

* **Fungsionalitas & Bukti Uji (40%)**:
  * Pengujian rangkaian hardware/simulator berjalan stabil.
  * Demonstrasi langsung di hadapan asisten laboratorium.
* **Analisis Data Tabel 3.1–3.2 & Jawaban C1–C6 (30%)**:
  * Kelengkapan pengisian data tabel pengujian sensor & aktuator.
  * Ketepatan dan kedalaman analisis teknis jawaban pertanyaan evaluasi C1–C6.
* **Kualitas & Kerapian Kode / Git (20%)**:
  * Struktur kode modular, penamaan variabel deskriptif, dan adanya komentar penjelas.
  * Riwayat *commit* jelas dan terdorong ke *branch* kelompok.
* **Format Laporan & Ketepatan Waktu (10%)**:
  * Sistematika pelaporan rapi dan pengumpulan tepat waktu sesuai batas lab.

---

### Panduan Demonstrasi Kepada Asisten Laboratorium

1. **Item 1 - Verifikasi Pin Aman & Kompilasi**:
   * Tunjukkan ke asisten bahwa program terkompilasi tanpa galat (*zero compilation error*).
   * Tunjukkan pinout ESP32 yang digunakan:
     * **GPIO 34**: ADC1 Input (sensor tanah / potensiometer) — *Bebas bentrok dengan modul Wi-Fi*.
     * **GPIO 4**: Output kontrol Relay modul pompa.
     * **GPIO 15**: Output PWM kipas pendingin / LED modulasi.
     * **GPIO 18**: Output LED Alarm proteksi *Dry-Run*.
     * **GPIO 26**: Input Push-Button internal *pull-up*.

2. **Item 2 - Moving Average & Inverse Mapping**:
   * Buka *Serial Monitor* pada baud rate `115200`.
   * Putar potensiometer/ubah sensor dari kering ke basah.
   * Tunjukkan bahwa nilai `Raw ADC` yang fluktuatif berhasil dihaluskan oleh `Filtered ADC`.
   * Tunjukkan bahwa saat ADC tinggi (kondisi kering/udara), persentase kelembaban bernilai **0%**. Saat ADC rendah (kondisi basah/air), persentase bernilai **100%** (Pemetaan terbalik).

3. **Item 3 - Demonstrasi Chattering vs Histeresis & PWM**:
   * **Mode Chattering (Ambang Tunggal 40%)**: Atur potensiometer tepat di sekitar 39% - 41%. Amati dan tunjukkan kepada asisten bahwa relai bergetar/berpindah status (*clicking*) secara agresif akibat *noise* analog.
   * **Mode Histeresis (Ambang Ganda 35% - 55%)**: Tekan tombol GPIO 26 untuk beralih mode. Tunjukkan bahwa pada rentang 35% s.d. 55% (*deadband/hysteresis loop*), status relai stabil dan tidak mengalami *chattering*.
   * **Modulasi PWM Kipas**: Amati perubahan nilai *duty cycle* PWM pada GPIO 15 (atau kecerahan LED) yang berubah dinamis dan halus proporsional terhadap kelembaban.

4. **Item 4 - Uji C6 Dry-Run Safety Cut-Off & Git Push**:
   * Posisikan potensiometer pada kondisi tanah kering (< 35%) sehingga relai pompa menyala (ON).
   * Biarkan potensiometer tetap kering selama 10 detik (mensimulasikan tandon air habis/kering).
   * Tunjukkan bahwa setelah 10 detik, sistem secara otomatis memutus relai (**CUT-OFF**), LED Merah GPIO 18 berkedip cepat (Alarm), dan sistem terkunci (*lockout*).
   * Tekan tombol GPIO 26 untuk mereset sistem ke kondisi normal.
   * Tunjukkan *commit log* Git pada *branch* kelompok di terminal VS Code: `git log --oneline`.
