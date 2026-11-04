/*
 * Perancangan Sistem Smart Lamp Berbasis ESP32 Menggunakan Sensor Suara
 * ---------------------------------------------------------------------
 * Fungsi  : Lampu menyala/mati dengan DUA KALI TEPUKAN (double clap).
 * Board   : ESP32 DevKit C
 * Sensor  : Sound sensor (KY-038 / small sound sensor) -> AO & DO
 * Output  : LED kuning (lampu), LED hijau (indikator tepukan)
 * Input   : Push button (saklar manual)
 */

// ---------- Konfigurasi pin ----------
#define PIN_SOUND_DO  27   // output digital sensor suara
#define PIN_SOUND_AO  34   // output analog sensor suara (ADC1)
#define PIN_LAMP      26   // LED lampu
#define PIN_STATUS    25   // LED indikator tepukan terdeteksi
#define PIN_BUTTON    14   // tombol manual (INPUT_PULLUP)
#define PIN_CLAP_SIM  13   // tombol simulasi tepukan (INPUT_PULLUP)

// ---------- Parameter deteksi tepuk ----------
const int  SOUND_ACTIVE_LEVEL = HIGH;   // ubah ke LOW jika sensor aktif-rendah
const unsigned long CLAP_DEBOUNCE_MS = 120;   // jeda minimum antar tepukan
const unsigned long CLAP_WINDOW_MS   = 1000;  // batas waktu tepukan ke-2
const unsigned long STATUS_BLINK_MS  = 100;   // lama LED indikator menyala
const unsigned long BUTTON_DEBOUNCE  = 50;

// ---------- Variabel status ----------
bool lampOn = false;
int  clapCount = 0;
unsigned long firstClapTime = 0;
unsigned long lastClapTime  = 0;
unsigned long statusOffTime = 0;

bool lastSoundState = !SOUND_ACTIVE_LEVEL;
bool lastButtonState = HIGH;
unsigned long lastButtonTime = 0;

void setLamp(bool state, const char *source) {
  lampOn = state;
  digitalWrite(PIN_LAMP, lampOn ? HIGH : LOW);
  Serial.printf("[LAMPU] %s  (sumber: %s)\n", lampOn ? "ON " : "OFF", source);
}

void flashStatus() {
  digitalWrite(PIN_STATUS, HIGH);
  statusOffTime = millis() + STATUS_BLINK_MS;
}

void handleSound() {
  unsigned long now = millis();
  // Suara terdeteksi dari sensor ATAU dari tombol simulasi tepukan
  bool soundState = (digitalRead(PIN_SOUND_DO) == SOUND_ACTIVE_LEVEL) ||
                    (digitalRead(PIN_CLAP_SIM) == LOW);

  // Deteksi tepi naik (suara baru terdeteksi)
  if (soundState && !lastSoundState && (now - lastClapTime > CLAP_DEBOUNCE_MS)) {
    lastClapTime = now;
    flashStatus();

    // Jika tepukan pertama sudah kedaluwarsa, mulai hitung ulang
    if (clapCount > 0 && (now - firstClapTime > CLAP_WINDOW_MS)) {
      clapCount = 0;
    }

    clapCount++;
    if (clapCount == 1) {
      firstClapTime = now;
      Serial.printf("[SUARA] Tepukan 1 | level analog = %d\n", analogRead(PIN_SOUND_AO));
    } else if (clapCount == 2) {
      Serial.printf("[SUARA] Tepukan 2 | selang = %lu ms\n", now - firstClapTime);
      setLamp(!lampOn, "double clap");
      clapCount = 0;
    }
  }
  lastSoundState = soundState;

  // Satu tepukan saja -> diabaikan setelah window habis
  if (clapCount == 1 && (now - firstClapTime > CLAP_WINDOW_MS)) {
    Serial.println("[SUARA] Hanya 1 tepukan, diabaikan");
    clapCount = 0;
  }
}

void handleButton() {
  bool btn = digitalRead(PIN_BUTTON);
  if (btn == LOW && lastButtonState == HIGH &&
      millis() - lastButtonTime > BUTTON_DEBOUNCE) {
    lastButtonTime = millis();
    setLamp(!lampOn, "tombol manual");
  }
  lastButtonState = btn;
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

  Serial.println("=== Smart Lamp ESP32 - Sensor Suara ===");
  Serial.println("Tepuk 2x untuk menyalakan/mematikan lampu.");
}

void loop() {
  handleSound();
  handleButton();

  if (statusOffTime && millis() >= statusOffTime) {
    digitalWrite(PIN_STATUS, LOW);
    statusOffTime = 0;
  }
}
