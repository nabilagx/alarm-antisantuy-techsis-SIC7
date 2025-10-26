/*
 * KODE PROYEK "ANTI-SANTUY" 
 * Oleh: Techsis
 *
 * * Logika Utama:
 * 1. ESP32 Nyala -> Otomatis masuk mode TIDUR (pantau LDR).
 * 2. LDR deteksi TERANG -> Alarm BUNYI (Buzzer + ASCII Kaget).
 * 3. Button ditekan -> Masuk CHALLENGE (Pencet 5x dlm 3 detik).
 * 4. Gagal Challenge -> LED HUKUMAN NYALA + Buzzer Nyala (ASCII Ejek).
 * 5. Sukses Challenge -> Alarm MATI total (ASCII Seneng).
 */

// --- KONFIGURASI PIN ---
#define LDR_DO_PIN   34  // Input Digital dari Modul LDR
#define LED_PIN      13  // Output LED Hukuman

#define BUZZER_PIN   18  // Output Audio
#define BUTTON_PIN   19  // Input Tombol

// --- KONFIGURASI PROYEK & AMBANG BATAS ---
const int LDR_TRIGGER_STATE = LOW; 
const int PRESSES_NEEDED = 5;
const int CHALLENGE_DURATION = 3000; // 3 detik

// --- Variabel buat State Machine ---
enum AlarmState { TIDUR, BUNYI, CHALLENGE, GAGAL, SUKSES };
AlarmState statusAlarm = TIDUR;
AlarmState lastStatusAlarm = (AlarmState)-1; 

// --- Variabel buat Challenge ---
int pressCount = 0;
unsigned long challengeStartTime = 0;

// --- Variabel buat Debouncing Tombol ---
bool buttonPressed = false;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;


void serialSleep() {
  Serial.println(F("\n================================="));
  Serial.println(F("STATUS: TIDUR"));
  Serial.println(F("   /\\_/\\"));
  Serial.println(F("  ( - . - )"));
  Serial.println(F("   >  ^  <"));
  Serial.println(F("zZz... (Memantau LDR)"));
  Serial.println(F("================================="));
}

void serialAwake() {
  Serial.println(F("\n================================="));
  Serial.println(F("STATUS: ALARM BUNYI!"));
  Serial.println(F("   /\\_/\\"));
  Serial.println(F("  ( o . o )"));
  Serial.println(F("   >  V  <"));
  Serial.println(F("BNGUUN!! (Tekan tombol!)"));
  Serial.println(F("================================="));
}

void serialChallenge() {
  Serial.println(F("\n================================="));
  Serial.println(F("STATUS: CHALLENGE!"));
  Serial.println(F("   /\\_/\\"));
  Serial.println(F("  ( o _ o )"));
  Serial.println(F("   >  -  <"));
  Serial.println(F("AYO TEKAN 5X DALAM 3 DETIK!"));
  Serial.println(F("================================="));
}

void serialFail() {
  Serial.println(F("\n================================="));
  Serial.println(F("STATUS: GAGAL! (WKWKWK!)"));
  Serial.println(F("   /\\_/\\"));
  Serial.println(F("  ( > . < )"));
  Serial.println(F("   >  ~  <"));
  Serial.println(F("HUKUMAN NYALA! (Tekan ampun)"));
  Serial.println(F("================================="));
}

void serialSuccess() {
  Serial.println(F("\n================================="));
  Serial.println(F("STATUS: SUKSES! (MANTUL!)"));
  Serial.println(F("   /\\_/\\"));
  Serial.println(F("  ( ^ . ^ )"));
  Serial.println(F("   >  U  <"));
  Serial.println(F("Selamat Pagi! Alarm Mati."));
  Serial.println(F("================================="));
}

/**
 * Fungsi helper buat print status ke Serial Monitor
 */
void printStatus(AlarmState newState) {
  switch (newState) {
    case TIDUR:
      serialSleep();
      break;
    case BUNYI:
      serialAwake();
      break;
    case CHALLENGE:
      serialChallenge();
      break;
    case GAGAL:
      serialFail();
      break;
    case SUKSES:
      serialSuccess();
      break;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Booting Alarm ANTI-SANTUY (v2.4 ASCII_ART)...");

  // Set mode pin
  pinMode(LDR_DO_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT); // Asumsi PULL-UP EKSTERNAL
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  // Pastiin semua aktuator mati di awal
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  Serial.println("Sistem siap!");
}

/**
 * Fungsi helper buat baca tombol DENGAN DEBOUNCING.
 */
bool readButtonPress() {
  bool pressed = false;
  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading == LOW && buttonPressed == false) {
      pressed = true;
      buttonPressed = true;
    } else if (reading == HIGH) {
      buttonPressed = false;
    }
  }
  lastButtonState = reading;
  return pressed;
}

void loop() {
  // Pindahkan deklarasi variabel ke sini (sebelum switch)
  int ldrDigitalValue = digitalRead(LDR_DO_PIN);
  unsigned long timePassed = 0;

  // Cek tombol 1x di awal loop
  bool tombolDitekan = readButtonPress();

  // Cuma nge-print status kalo ada perubahan state
  // Biar serial monitor nggak banjir (spam)
  if (statusAlarm != lastStatusAlarm) {
    printStatus(statusAlarm); // Panggil fungsi ASCII Art
    lastStatusAlarm = statusAlarm; // Update status terakhir
  }

  switch (statusAlarm) {

    case TIDUR:
      // Cek kalo udah terang
      if (ldrDigitalValue == LDR_TRIGGER_STATE) {
        Serial.println("-> LDR: FAJAR TERDETEKSI!");
        statusAlarm = BUNYI;
      }
      delay(200); // Jeda pengecekan LDR
      break;

    case BUNYI:
      digitalWrite(BUZZER_PIN, HIGH); // Nyalain buzzer

      // Cek kalo tombol "snooze" ditekan
      if (tombolDitekan) {
        Serial.println("-> Tombol ditekan! Challenge Dimulai!");
        digitalWrite(BUZZER_PIN, LOW); // Matiin buzzer
        pressCount = 0;
        challengeStartTime = millis();
        statusAlarm = CHALLENGE;
      }
      break;

    case CHALLENGE:
      // Kalo tombol ditekan, tambahin counternya
      if (tombolDitekan) {
        pressCount++;
        Serial.print("-> Pencetan ke: "); Serial.println(pressCount);
      }

      timePassed = millis() - challengeStartTime;

      if (pressCount >= PRESSES_NEEDED) {
        Serial.println("-> SUKSES! (5x pencet tercapai)");
        statusAlarm = SUKSES;
      }
   
      else if (timePassed > CHALLENGE_DURATION) {
        Serial.println("-> WAKTU HABIS! GAGAL!");
        statusAlarm = GAGAL;
      }

      break;

    case GAGAL:
      digitalWrite(LED_PIN, HIGH);    // Hukuman LED
      digitalWrite(BUZZER_PIN, HIGH); // Hukuman Buzzer

      // "TOMBOL AMPUN"
      if (tombolDitekan) {
        Serial.println("-> DIAMPUNI! Balik tidur...");
        digitalWrite(LED_PIN, LOW);
        digitalWrite(BUZZER_PIN, LOW);
        statusAlarm = TIDUR;
      }
      break;

    case SUKSES:
      digitalWrite(LED_PIN, LOW); 
      digitalWrite(BUZZER_PIN, LOW);
      
      while (true) {
        delay(10000); 
      }
      break;
  }
}
