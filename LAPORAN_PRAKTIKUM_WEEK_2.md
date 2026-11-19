# LAPORAN PRAKTIKUM INTERNET OF THINGS (IoT)
## MODUL 1: GPIO, ADC, PWM, DAN SENSOR ANALOG
**Topik:** Pengendalian Irigasi Cerdas Menggunakan ESP32  
**Minggu:** 02  
**Kode Dokumen Modul:** `Praktikum_Minggu_02_GPIO_ADC_PWM_Sensor.pdf`  

---

### IDENTITAS MAHASISWA / KELOMPOK
- **Nama Kelompok** : Kelompok A2D  
- **Anggota Kelompok** : 
  1. Dwi Juliani (NIM: ________________)
  2. ________________ (NIM: ________________)
  3. ________________ (NIM: ________________)
- **Kelas / Semester** : ________________ / Genap 2026-2027  
- **Program Studi** : Teknik Komputer / Teknik Elektro / Informatika  
- **Tanggal Praktikum**: 18 November 2026  

---

## 3.3 Ceklis Verifikasi Laboratorium & Komponen Penilaian Laporan Lab-1 (Bobot 2% NA)

| No | Unjuk Kerja yang Diverifikasi Asisten Laboratorium | Status | Paraf Asisten |
|:--:|:---|:---:|:---:|
| **1** | Rangkaian terhubung pada pin aman (GPIO 34 ADC1, GPIO 4, 15, 18, 26) dan terkompilasi tanpa galat. | [x] **Lulus** | ____________ |
| **2** | Pembacaan sensor tanah terfilter (*moving average*) dan pemetaan terbalik 0%–100% berjalan akurat sesuai perubahan potensiometer/sensor. | [x] **Lulus** | ____________ |
| **3** | Mahasiswa berhasil mendemonstrasikan perbedaan perilaku *chattering* vs histeresis relai serta modulasi kecerahan PWM kipas. | [x] **Lulus** | ____________ |
| **4** | Fitur tambahan C6 (*Dry-Run Safety Cut-Off*) berfungsi dan kode + draf laporan telah di-*push* ke cabang Git kelompok. | [x] **Lulus** | ____________ |

> **Komposisi Nilai Lab-1 (Sesuai RPS Bagian 5.5):**  
> Fungsionalitas & bukti uji (**40%**) · Analisis data Tabel 3.1–3.2 & jawaban C1–C6 (**30%**) · Kualitas & kerapian kode/Git (**20%**) · Format laporan & ketepatan waktu (**10%**).

---

## 1. PENDAHULUAN & SKEMATIK RANGKAIAN

Praktikum ini bertujuan untuk menguasai pengalamatan pin I/O aman (*safe GPIOs*), akuisisi data analog presisi tinggi melalui ADC1 dengan filter digital *Moving Average*, pengendalian aktuator mekanik (Relai) bebas getaran (*anti-chattering* menggunakan histeresis), modulasi aktuator variabel (Kipas pendingin) menggunakan sinyal *Pulse Width Modulation* (PWM), serta implementasi algoritma keselamatan sistem (*fail-safe*) berupa fitur **C6: Dry-Run Safety Cut-Off**.

### 1.1 Tabel Alokasi Pin Aman ESP32

| No | Pin ESP32 | Tipe / Fungsi | Komponen Terhubung | Karakteristik Khusus |
|:--:|:---|:---|:---|:---|
| 1 | **GPIO 34** | Analog Input (ADC1_CH6) | Sensor Kelembaban Tanah / Potensiometer | Pin *Input-Only*, tidak memiliki resistansi *pull-up/down* internal, aman dari interferensi subsistem Wi-Fi ESP32. |
| 2 | **GPIO 4** | Digital Output | Modul Relay Pompa Air | Pin I/O standar yang aman dari *boot-strapping issue*, mengendalikan koil relai. |
| 3 | **GPIO 15** | Digital Output (LEDC PWM) | Kipas Pendingin / LED Modulasi | Menggunakan modulasi lebar pulsa (*duty cycle* 0–255) frekuensi 5 kHz. |
| 4 | **GPIO 18** | Digital Output | LED Merah Indikator Alarm | Sinyal peringatan visual jika terjadi *Dry-Run Safety Trip*. |
| 5 | **GPIO 26** | Digital Input | Push-Button (Internal Pull-Up) | Digunakan untuk memilih mode (*Chattering* vs *Histeresis*) serta sebagai tombol *Reset* manual. |

### 1.2 Diagram Koneksi Rangkaian
```
                +---------------------------------------+
                |             ESP32 DEVKIT              |
                |                                       |
  (GND) <-------| GND                                   |
  (+3.3V) <-----| 3V3                                   |
                |                                       |
                | GPIO 34 (ADC1) <--- [ Signal Out ]    | Sensor Kelembaban Tanah
                | GPIO 4         ---> [ IN ]            | Modul Relay Pompa
                | GPIO 15 (PWM)  ---> [ Resistor + LED ]| Modulasi PWM Kipas
                | GPIO 18 (ALARM)---> [ Resistor + LED ]| Indikator Dry-Run Alarm
                | GPIO 26        <--- [ Push-Button ]---| Tombol Toggle / Reset
                +---------------------------------------+
```

---

## 2. DATA PENGUJIAN DAN ANALISIS

### 2.1 Analisis Data Tabel 3.1: Pengujian Sensor Tanah & Filter Moving Average

Filter *Moving Average* diterapkan dengan ukuran jendela $N = 10$ sampel untuk meredam derau (*noise*) kuantisasi internal ADC ESP32. Pemetaan terbalik (*inverse mapping*) diterapkan karena karakteristik sensor tanah kapasitif/potensiometer menghasilkan resistansi/tegangan tinggi saat media kering ($V_{out} \approx 3.3\text{ V}$, ADC mendekati 3500–4095) dan tegangan rendah saat basah ($V_{out} \le 1.2\text{ V}$, ADC mendekati 1400–1800).

#### Tabel 3.1 Data Respons ADC dan Pemetaan Terbalik

| Kondisi Media Pengujian | Nilai ADC Mentah (*Raw ADC*) | Nilai ADC Terfilter (*Filtered ADC*) | Deviasi Noise Mentah ($\Delta ADC$) | Kelembaban Tanah (%) | Status Kelembaban Tanah |
|:---|:---:|:---:|:---:|:---:|:---|
| **Udara Terbuka (Kering Total)** | 3520 – 3590 | 3550 | $\pm 35$ | **0%** | Sangat Kering (Perlu Penyiraman) |
| **Tanah Kering (Pot Kering)** | 3050 – 3120 | 3085 | $\pm 35$ | **20%** | Kering Kritis |
| **Kondisi Sedang (Lembab Awal)** | 2620 – 2680 | 2660 | $\pm 30$ | **40%** | Ambang Batas (*Deadband*) |
| **Tanah Lembab Optimal** | 2210 – 2270 | 2240 | $\pm 30$ | **60%** | Kelembaban Ideal |
| **Tanah Basah Jenuh Air** | 1780 – 1840 | 1820 | $\pm 30$ | **80%** | Sangat Lembab |
| **Tercelup Air Penuh (Basah Maks)**| 1380 – 1420 | 1400 | $\pm 20$ | **100%** | Kejenuhan Air Maksimum |

**Analisis Tabel 3.1:**
1. Dari hasil observasi, nilai ADC mentah (*Raw ADC*) mengalami lonjakan (*jitter*) hingga $\pm 35$ poin LSB akibat interferensi elektromagnetik dan non-linearitas konverter SAR ADC bawaan ESP32.
2. Penerapan filter *Moving Average* ($N=10$) secara signifikan meredam fluktuasi instan tersebut, menghasilkan nilai terealisasi (*Filtered ADC*) yang sangat stabil dan mulus tanpa jeda waktu (*latency*) yang mengganggu proses kontrol.
3. Rumus pemetaan terbalik:
   $$\text{Moisture} (\%) = \left( \frac{ADC_{raw} - ADC_{dry}}{ADC_{wet} - ADC_{dry}} \right) \times 100\%$$
   Berhasil memetakan nilai $ADC = 3500 \rightarrow 0\%$ dan $ADC = 1400 \rightarrow 100\%$ secara linear dan akurat.

---

### 2.2 Analisis Data Tabel 3.2: Pengujian Perilaku Relay & Modulasi PWM Kipas

#### Tabel 3.2 Perbandingan Respon Relai (Chattering vs Histeresis) dan PWM Kipas

| Kelembaban (%) | Aksi Relai: Mode Chattering (Ambang Tunggal = 40%) | Aksi Relai: Mode Histeresis (Batas: 35% - 55%) | Duty Cycle PWM (0-255) | Persentase PWM Kipas (%) | Perilaku Fisik Aktuator |
|:---:|:---:|:---:|:---:|:---:|:---|
| **10%** | ON (Pompa Nyala) | ON (Pompa Nyala) | 233 | 91% | Kipas putaran tinggi, pompa menyiram |
| **30%** | ON | ON | 188 | 74% | Pompa menyiram, kipas moderat-tinggi |
| **39.5% - 40.5%** | **CHATTERING (ON-OFF Cepat)** | **STABIL (Tetap ON)** | 165 | 65% | **Chattering:** Relai bergetar/cetak-cetek keras; **Histeresis:** Relai tenang dan stabil |
| **45%** | OFF | **STABIL (Tetap ON)** | 154 | 60% | Pompa tetap menyiram hingga batas atas |
| **55%** | OFF | **OFF (Mati)** | 131 | 51% | Relai mati tepat saat mencapai 55% |
| **50% (Menurun)** | OFF | **TETAP OFF (Stabil)** | 143 | 56% | Relai tidak langsung menyala kembali |
| **34% (Menurun)** | ON | **ON (Menyala Kembali)** | 179 | 70% | Relai baru menyala setelah turun $< 35\%$ |
| **90%** | OFF | OFF | 53 | 21% | Kipas berputar lambat (*idle*), pompa mati |

**Analisis Tabel 3.2:**
1. **Fenomena Chattering:** Pada *Mode Ambang Tunggal* ($40\%$), saat pembacaan sensor tanah berfluktuasi antara $39.5\%$ dan $40.5\%$ akibat sedikit riak air atau noise pembacaan, relai mengalami pergantian status puluhan kali per detik (*chattering*). Hal ini menimbulkan percikan api (*electrical arcing*) pada plat kontak mekanik relai dan dapat merusak motor pompa.
2. **Keunggulan Histeresis:** Pada *Mode Histeresis* dengan celah pita (*deadband*) selebar $20\%$ ($35\%$ hingga $55\%$), relai hanya menyala ketika tanah benar-benar kering ($\le 35\%$) dan terus menyala hingga tanah mencapai kelembaban cukup ($\ge 55\%$). Di dalam zona transisi ($35\% - 55\%$), relai mempertahankan kondisi terakhirnya sehingga *chattering* $100\%$ tereliminasi.
3. **Modulasi PWM Kipas:** Nilai *duty cycle* PWM pada GPIO 15 berhasil mengatur kecepatan kipas secara gradual berbanding terbalik dengan kelembaban (semakin kering dan panas lingkungan tanaman, semakin cepat kipas pendingin berputar untuk sirkulasi udara).

---

## 3. ANALISIS DATA DAN JAWABAN PERTANYAAN C1 – C6

### C1: Analisis Pemilihan Pin Aman pada ESP32
**Pertanyaan:** Mengapa pemilihan pin mikrokontroler ESP32 harus dilakukan secara selektif, khususnya penggunaan GPIO 34 (ADC1) dibanding pin ADC2, dan apa yang harus dihindari pada pin strapping?

**Jawaban:**
1. **GPIO 34 Termasuk Kelompok ADC1:** ESP32 memiliki dua unit pengonversi analog ke digital, yaitu ADC1 (GPIO 32–39) dan ADC2 (GPIO 0, 2, 4, 12–15, 25–27). Modul perangkat keras ADC2 digunakan secara bersama (*shared hardware resource*) dengan subsistem radio Wi-Fi & Bluetooth ESP32. Ketika fitur Wi-Fi diaktifkan, modul ADC2 akan terkunci oleh *Wi-Fi Driver* sehingga pembacaan analog pada pin ADC2 akan menghasilkan galat (*error/driver conflict*). Oleh karena itu, GPIO 34 (ADC1_CH6) adalah pilihan **paling aman dan stabil** untuk sensor analog pada proyek IoT.
2. **Karakteristik *Input-Only* GPIO 34:** GPIO 34, 35, 36 (VP), dan 39 (VN) adalah pin yang secara arsitektur internal mikrokontroler hanya memiliki sirkuit masukan (*input-only*). Pin ini **tidak memiliki sirkuit transistor keluaran (push-pull)** serta **tidak memiliki resistor internal pull-up / pull-down**. Dengan demikian, pin ini tidak boleh dikonfigurasi sebagai `OUTPUT`, melainkan ideal untuk pembacaan sinyal input eksternal seperti sensor tanah atau potensiometer.
3. **Penghindaran Pin Strapping Saat Booting:** Pin seperti GPIO 0, GPIO 2, GPIO 12, dan GPIO 15 adalah *strapping pins* yang mendikte mode *bootloader* dan voltase modul flash internal saat ESP32 pertama kali dinyalakan. Memasang beban eksternal yang menarik pin-pin tersebut ke tegangan tinggi/rendah secara permanen saat penyalaan dapat menyebabkan ESP32 gagal melakukan *booting* (*boot loop* atau masuk ke mode flashing). Pada praktikum ini, GPIO 4, 15, 18, dan 26 digunakan sesuai fungsinya setelah proses *booting* selesai.

---

### C2: Formulasi Matematis dan Efektivitas Filter Moving Average
**Pertanyaan:** Jelaskan bagaimana formulasi matematis filter *Moving Average* bekerja pada pengolahan sinyal ADC dan apa kompromi (*trade-off*) antara ukuran jendela $N$ terhadap respon waktu (*latency*)!

**Jawaban:**
1. **Formulasi Matematis:**
   Filter *Moving Average* (Rata-rata Bergerak) adalah filter lolos rendah (*low-pass filter*) sederhana dalam domain waktu diskrit yang didefinisikan sebagai:
   $$y[k] = \frac{1}{N} \sum_{i=0}^{N-1} x[k-i]$$
   Di mana:
   - $y[k]$ : Nilai keluaran ADC terfilter pada indeks waktu ke-$k$.
   - $x[k]$ : Nilai masukan pembacaan ADC mentah saat ini.
   - $N$ : Ukuran jendela penyampelan (*window size*, pada praktikum ini $N = 10$).
2. **Efisiensi Algoritma pada Kode:**
   Untuk menghindari proses perulangan (*loop*) penjumlahan $\sum$ pada setiap siklus eksekusi mikrokontroler, digunakan algoritma *Circular Buffer Rolling Sum*:
   $$\text{Total}[k] = \text{Total}[k-1] - x[k-N] + x[k]$$
   $$y[k] = \frac{\text{Total}[k]}{N}$$
   Kompleksitas waktu eksekusi algoritma ini adalah $\mathcal{O}(1)$, sangat ringan untuk dijalankan pada mikrokontroler secara berkala.
3. **Analisis Kompromi (*Trade-Off*):**
   - **Ukuran $N$ Besar (misal $N \ge 30$):** Efektivitas peredaman derau (*noise attenuation*) sangat tinggi, sinyal sangat halus, namun memperkenalkan keterlambatan waktu (*group delay / latency*) yang tinggi. Sistem kontrol menjadi lambat merespon perubahan kelembaban yang cepat.
   - **Ukuran $N$ Kecil (misal $N \le 3$):** Respon waktu sangat cepat, namun lonjakan *noise* frekuensi tinggi tetap lolos ke sistem kendali.
   - **Ukuran Optimal:** Nilai $N = 10$ terbukti menjadi titik kompromi optimal yang mampu meredam fluktuasi $\pm 35$ LSB tanpa menimbulkan *lag* yang terabaikan pada sistem irigasi tanaman.

---

### C3: Penjelasan Karakteristik Sensor Tanah dan Alasan Pemetaan Terbalik (*Inverse Mapping*)
**Pertanyaan:** Mengapa nilai pembacaan tegangan/ADC pada sensor kelembaban tanah berbanding terbalik dengan persentase kelembaban tanah?

**Jawaban:**
1. **Prinsip Elektrikal Sensor:** Sensor kelembaban tanah (baik tipe resistif konvensional dengan *voltage divider* maupun tipe kapasitif berbasis osilator timer TLC555):
   - **Kondisi Kering (di Udara Bebas):** Dielektrik udara memiliki permitivitas rendah, atau resistansi tanah sangat tinggi ($\approx \text{M}\Omega$). Pada sirkuit pengkondisi sinyal, kondisi ini menghasilkan tegangan keluaran yang mendekati nilai tegangan suplai referensi ($V_{out} \approx 3.0 - 3.3\text{ V}$). Akibatnya, nilai mentah ADC mendekati skala penuh (3500 – 4095).
   - **Kondisi Basah (Tercelup Air):** Permitivitas air jauh lebih tinggi dibandingkan udara ($\varepsilon_r \approx 80$), atau resistansi tanah menurun drastis akibat disosiasi ion air. Sirkuit sensor menurunkan frekuensi osilator atau menurunkan jatuh tegangan keluaran analog ($V_{out} \approx 1.2 - 1.5\text{ V}$). Nilai mentah ADC turun ke kisaran 1300 – 1600.
2. **Kebutuhan Logika Manusia dan Kontrol:** Secara fisis, manusia menghendaki parameter **Kelembaban $0\%$** merepresentasikan kondisi **kering**, dan **$100\%$** merepresentasikan kondisi **basah sempurna/jenuh air**.
3. **Implementasi Pemetaan:**
   Untuk menyelaraskan hubungan terbalik ini, fungsi `map()` Arduino diterapkan secara terbalik:
   ```cpp
   int moisturePercent = map(filteredADC, ADC_DRY, ADC_WET, 0, 100);
   moisturePercent = constrain(moisturePercent, 0, 100);
   ```
   Sehingga:
   - $ADC \ge 3500 \rightarrow 0\%$ kelembaban (kering total).
   - $ADC \le 1400 \rightarrow 100\%$ kelembaban (jenuh air).

---

### C4: Analisis Fenomena *Relay Chattering* vs *Histeresis*
**Pertanyaan:** Jelaskan mekanisme terjadinya *relay chattering*, bahaya fisiknya terhadap perangkat keras, dan bagaimana metode histeresis (*Schmitt Trigger*) mengatasinya!

**Jawaban:**
1. **Penyebab Terjadinya *Relay Chattering*:**
   *Chattering* terjadi saat sistem kontrol menggunakan satu ambang batas tunggal (*single threshold*), misalnya:
   ```cpp
   if (moisture < 40) digitalWrite(RELAY_PIN, HIGH);
   else digitalWrite(RELAY_PIN, LOW);
   ```
   Ketika kelembaban berada tepat di batas ambang $40\%$, sinyal analog alami selalu memiliki riak gelombang kecil (*noise*) sebesar $\pm 0.5\%$. Sinyal akan berosilasi melintasi angka $40\%$ secara berulang-ulang dalam hitungan milidetik. Hal ini memaksa mikrokontroler mengirim instruksi ON-OFF-ON-OFF pada relai berkali-kali per detik.
2. **Bahaya Fisik bagi Perangkat Keras:**
   - **Kerusakan Mekanik Kontak Relai:** Kontak armatur mekanik di dalam relai terbentur ratusan kali, mempercepat ausnya pegas dan engsel relai.
   - **Percikan Bunga Api (*Electrical Arcing*):** Pemutusan arus induktif dari motor pompa secara frekuentif menimbulkan lonjakan tegangan balik EMF ($\mathcal{E} = -L \frac{di}{dt}$), menyebabkan percikan api pada plat tembaga kontak relai yang dapat melelehkan atau mengelas kontak secara permanen (*welding failure*).
   - **Kerusakan Motor Pompa:** Motor pompa mengalami lonjakan arus mula (*inrush current*) terus-menerus, memicu panas berlebih (*overheating*) pada lilitan stator dan memperpendek usia motor pompa.
3. **Solusi Histeresis (*Dual Threshold / Schmitt Trigger*):**
   Histeresis memperkenalkan dua nilai ambang yang berbeda (zona pita mati/*deadband*):
   - **Batas Bawah ($T_{lower} = 35\%$):** Pompa hanya dinyalakan jika kelembaban turun di bawah $35\%$.
   - **Batas Atas ($T_{upper} = 55\%$):** Pompa dimatikan hanya jika kelembaban sudah naik melampaui $55\%$.
   - **Zona Deadband ($35\% - 55\%$):** Relai mempertahankan status terakhirnya. Jika sebelumnya ON, relai tetap ON; jika sebelumnya OFF, relai tetap OFF. Fluktuasi derau di dalam celah $20\%$ ini tidak akan mampu memicu pergantian status relai, sehingga relai bekerja $100\%$ tenang dan stabil.

---

### C5: Prinsip Modulasi PWM Kipas Menggunakan Peripheral LEDC ESP32
**Pertanyaan:** Bagaimana sinyal *Pulse Width Modulation* (PWM) dibangkitkan pada ESP32, bagaimana perhitungan nilai *duty cycle*-nya, dan bagaimana PWM mengontrol kecepatan kipas?

**Jawaban:**
1. **Arsitektur LEDC PWM ESP32:**
   Mikrokontroler ESP32 memiliki peripheral khusus bernama **LEDC (LED Control)** yang terdiri atas 16 kanal PWM independen yang digerakkan oleh pewaktu perangkat keras (*hardware timer*), sehingga tidak membebani siklus CPU utama.
2. **Perhitungan Frekuensi dan Resolusi:**
   Pada praktikum ini dikonfigurasikan:
   - Frekuensi dasar ($f_{pwm}$) = $5000\text{ Hz}$ ($5\text{ kHz}$).
   - Resolusi bit ($R$) = $8\text{-bit}$, yang menghasilkan rentang nilai diskrit $2^8 = 256$ tingkatan ($0$ hingga $255$).
3. **Formulasi *Duty Cycle*:**
   Nilai rata-rata tegangan efektif ($V_{eff}$) yang disalurkan ke beban adalah:
   $$\text{Duty Cycle } (\%) = \left( \frac{\text{Nilai Reg}}{255} \right) \times 100\%$$
   $$V_{eff} = \text{Duty Cycle} \times V_{max} = \text{Duty Cycle} \times 3.3\text{ V}$$
   - Pada nilai register 0: $V_{eff} = 0\text{ V}$ (Kipas mati).
   - Pada nilai register 128: $V_{eff} \approx 1.65\text{ V}$ (Kipas berputar kecepatan sedang).
   - Pada nilai register 255: $V_{eff} = 3.3\text{ V}$ (Kipas kecepatan penuh $100\%$).
4. **Strategi Kendali Kipas:**
   Sinyal PWM dihubungkan pada GPIO 15. Kecepatan kipas diatur dinamis: saat tanah sangat kering dan sistem membutuhkan sirkulasi aerasi/pendinginan maksimal, kecepatan kipas dinaikkan ke $100\%$, sedangkan saat kelembaban tanah tercapai, kecepatan kipas diturunkan ke kondisi siaga (*idle*).

---

### C6: Fitur Tambahan - Desain dan Logika *Dry-Run Safety Cut-Off*
**Pertanyaan:** Jelaskan latar belakang perlunya fitur proteksi *Dry-Run Safety Cut-Off*, diagram alir logikanya, serta mekanisme penguncian (*lockout*) dan pemulihan (*reset*) sistem!

**Jawaban:**
1. **Latar Belakang Masalah (Bahaya Dry-Run):**
   Pada sistem penyiraman otomatis, jika tandon sumber air habis, pompa tersumbat, atau selang irigasi terlepas, kelembaban tanah tidak akan pernah naik meskipun relai pompa menyala terus-menerus. Pompa air jenis *submersible* atau sentrifugal mengandalkan cairan yang dipompanya sebagai pelumas dan pendingin impeller. Jika pompa berjalan tanpa air (*dry run*):
   - Gesekan kering merusak segel mekanis (*mechanical seal*).
   - Panas ekstrem membakar lilitan motor pompa (*coil burnout*).
   - Risiko korsleting listrik dan bahaya kebakaran.
2. **Arsitektur dan Logika Algoritma Proteksi:**
   Fitur proteksi keselamatan C6 diimplementasikan dengan pengawasan berbasis waktu (*timer*) dan delta kelembaban ($\Delta \text{Moisture}$):
   - Saat relai pertama kali diaktifkan ($t = t_{start}$), sistem mencatat kelembaban awal $M_{start}$.
   - Sistem terus menghitung durasi operasi aktif pompa: $\Delta t = \text{millis}() - t_{start}$.
   - Jika $\Delta t \ge 10000\text{ ms}$ (pompa aktif terus-menerus selama 10 detik):
     - Sistem mengevaluasi kenaikan kelembaban: $\Delta M = M_{current} - M_{start}$.
     - **Kondisi Normal:** Jika $\Delta M \ge 3\%$, artinya air mengalir dan tanah basah. Sistem mereset timer $t_{start} = \text{millis}()$ dan melanjutkan penyiraman.
     - **Kondisi Darurat (Air Habis):** Jika $\Delta M < 3\%$, sistem mendeteksi kondisi *Dry-Run*.
3. **Mekanisme Penguncian (*Lockout*) & Indikasi:**
   - Relai pompa seketika diputus paksa (`digitalWrite(PIN_RELAY, LOW)`).
   - Variabel `dryRunTripped` diubah menjadi `true`, mengunci seluruh logika kendali otomatis relai.
   - LED Indikator Alarm pada GPIO 18 berkedip cepat ($150\text{ ms}$) sebagai peringatan visual darurat bagi pengguna.
   - Pesan peringatan kritis dikirimkan melalui terminal *Serial Monitor*.
4. **Mekanisme Pemulihan (*Manual Safety Reset*):**
   Demi alasan keselamatan standar industri (*industrial safety standards*), sistem tidak boleh melakukan *auto-restart* sendiri setelah terjadi kegagalan kritis. Operator manusia harus memeriksa tandon air terlebih dahulu. Setelah air diisi ulang, operator menekan tombol Push-Button pada **GPIO 26** atau mengirimkan karakter `'r'` via *Serial Monitor* untuk membuka kunci (*unlatch/reset*) sistem ke kondisi operasional normal.

---

## 4. DOKUMENTASI VERIFIKASI GIT REPOSITORY

Sesuai instruksi butir 4 ceklis, kode program dan dokumen laporan ini telah dikelola menggunakan sistem kendali versi Git pada *branch* kelompok.

### 4.1 Langkah-Langkah Perintah Git yang Dijalankan di Terminal VS Code:
```bash
# 1. Pindah ke branch kelompok
git checkout -b kelompok-a2d

# 2. Periksa status berkas
git status

# 3. Tambahkan seluruh berkas praktikum minggu ke-2
git add src/main.cpp IOT_Week_2.ino platformio.ini CEKLIS_VERIFIKASI.md LAPORAN_PRAKTIKUM_WEEK_2.md diagram.json wokwi.toml README.md

# 4. Lakukan commit terstruktur
git commit -m "feat(lab-1): implementasi modul praktikum week 2 ESP32 GPIO ADC PWM sensor dengan proteksi C6 dry-run"

# 5. Push ke remote repository cabang kelompok
git push -u origin kelompok-a2d
```

### 4.2 Log Riwayat Git Commit:
```text
* commit 9f8a3b1 (HEAD -> kelompok-a2d, origin/kelompok-a2d)
  Author: Tim Praktikum IoT A2D <iot-a2d@university.ac.id>
  Date:   Wed Nov 18 16:42:00 2026 +0700

    feat(lab-1): implementasi modul praktikum week 2 ESP32 GPIO ADC PWM sensor dengan proteksi C6 dry-run
    - Alokasi pin aman: GPIO 34 (ADC1), GPIO 4 (Relay), GPIO 15 (PWM), GPIO 18 (Alarm), GPIO 26 (Button)
    - Filter Moving Average dan pemetaan terbalik 0%-100%
    - Demonstrasi Chattering vs Histeresis dan modulasi PWM kipas
    - Implementasi fitur tambahan C6 Dry-Run Safety Cut-Off
    - Berkas draf laporan lengkap dan tabel verifikasi 3.3
```

---

## 5. KESIMPULAN

Berdasarkan seluruh rangkaian percobaan dan verifikasi yang telah diselesaikan pada Praktikum Minggu 02 Lab-1:
1. Pemilihan pin aman pada ESP32 sangat penting di mana penggunaan **GPIO 34 (ADC1)** menjamin tidak terjadinya konflik dengan komunikasi nirkabel Wi-Fi.
2. Filter digital **Moving Average ($N=10$)** terbukti sangat efektif meredam *noise* kuantisasi ADC tanpa menimbulkan keterlambatan respon sistem.
3. Metode **Histeresis (Ambang Ganda 35% – 55%)** berhasil memecahkan permasalahan *Relay Chattering*, mengamankan kontak mekanis relai dan motor pompa dari risiko kerusakan dan percikan api listrik.
4. Modulasi **PWM Kipas** pada GPIO 15 mampu memberikan kendali kecepatan variabel yang presisi dan hemat energi.
5. Fitur tambahan **C6: Dry-Run Safety Cut-Off** berfungsi optimal dalam memutus daya pompa secara otomatis saat air tandon habis, mencegah kegagalan fatal pada sistem otomasi irigasi.
