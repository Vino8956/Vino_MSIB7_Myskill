#define BLYNK_TEMPLATE_ID "TMPL6vuF4lNlf"
#define BLYNK_TEMPLATE_NAME "Final Project"
#define BLYNK_AUTH_TOKEN "9hPu6uKfre99Pz_VWWYGqTYoYZzXzpHU"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// Informasi WiFi
const char* ssid = "kos 23";    // Nama WiFi
const char* pass = "57584321";   // Password WiFi

// Pin konfigurasi
const int pinMQ = 34;    // Pin analog MQ135
const int pinMerah = 25; // Pin LED merah
const int pinHijau = 26; // Pin LED hijau
const int pinBiru = 27;  // Pin LED biru
const int pinBzr = 32;   // Pin buzzer
const int pinRly = 33;   // Pin relay kipas

// Variabel kalibrasi
float RL = 10.0;          // Nilai resistor load
float R0 = 76.63;         // Nilai RZERO hasil kalibrasi
float kurvaCO2[3] = {2.602, 0.2, -0.48}; // Kurva kalibrasi CO2

// Variabel waktu dengan millis()
unsigned long waktuBzrSblm = 0;
unsigned long intervalBzr = 200;
unsigned long waktuMQSblm = 0;
unsigned long intervalMQ = 1000;
unsigned long waktuNotifSblm = 0;
unsigned long intervalNotif = 2000;
bool statusBzr = false;

// Variabel kontrol manual
bool controlfan = false; // Status kontrol manual relay

WidgetLED Merah(V1);
WidgetLED Kuning(V2);
WidgetLED Hijau(V3);

void setup() {
  Serial.begin(115200);

  // Koneksi WiFi dan Blynk
  WiFi.begin(ssid, pass);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Konfigurasi pin
  pinMode(pinMerah, OUTPUT);
  pinMode(pinHijau, OUTPUT);
  pinMode(pinBiru, OUTPUT);
  pinMode(pinBzr, OUTPUT);
  pinMode(pinRly, OUTPUT);

  // Preheat sensor MQ-135 selama 1 menit
  Serial.println("Preheating MQ-135...");
  for (int i = 60; i > 0; i--) {
    Serial.print("Waktu preheat: ");
    Serial.print(i);
    Serial.println(" detik");
    delay(1000);
  }
  Serial.println("Preheat selesai. Inisialisasi CO2 Detector");
}

void loop() {
  Blynk.run();

  unsigned long waktuSaatIni = millis();

  // Bacaan sensor
  static float ppmCO2 = 0;
  if (waktuSaatIni - waktuMQSblm >= intervalMQ) {
    waktuMQSblm = waktuSaatIni;
    int adc = analogRead(pinMQ);
    ppmCO2 = hitungPPM(adc);

    Serial.print("CO2 (ppm): ");
    Serial.println(ppmCO2);

    // Kirim data ke Blynk
    Blynk.virtualWrite(V0, ppmCO2);

    if (!controlfan) {
      RGB_Relay(ppmCO2, waktuSaatIni);
    }
  }

  // Kontrol buzzer
  buzzer(ppmCO2, waktuSaatIni);
}

void RGB_Relay(float ppmCO2, unsigned long waktuSaatIni) {
  if (ppmCO2 <= 750) {
    RGB(0, 1, 0);
    digitalWrite(pinRly, HIGH);
    Merah.off();
    Kuning.off();
    Hijau.on();
    Blynk.virtualWrite(V5, LOW);
  } else if (ppmCO2 > 750 && ppmCO2 <= 1200) {
    RGB(1, 1, 0);
    digitalWrite(pinRly, LOW);
    Merah.off();
    Kuning.on();
    Hijau.off();
    Blynk.virtualWrite(V5, HIGH);
    if (waktuSaatIni - waktuNotifSblm >= intervalNotif) {
      Blynk.logEvent("WARNING", "CO2 meningkat");
      waktuNotifSblm = waktuSaatIni;
    }
  } else {
    RGB(1, 0, 0);
    digitalWrite(pinRly, LOW);
    Merah.on();
    Kuning.off();
    Hijau.off();
    Blynk.virtualWrite(V5, HIGH);
    if (waktuSaatIni - waktuNotifSblm >= intervalNotif) {
      Blynk.logEvent("DANGER", "CO2 sangat tinggi!");
      waktuNotifSblm = waktuSaatIni;
    }
  }
}

void buzzer(float ppmCO2, unsigned long waktuSaatIni) {
  if (ppmCO2 > 750) {
    if (waktuSaatIni - waktuBzrSblm >= intervalBzr) {
      waktuBzrSblm = waktuSaatIni;
      statusBzr = !statusBzr;
      int freq = (ppmCO2 <= 1200) ? 1000 : 2000;
      if (statusBzr) {
        tone(pinBzr, freq);
        Blynk.virtualWrite(V4, HIGH);
      } else {
        noTone(pinBzr);
        Blynk.virtualWrite(V4, LOW);
      }
    }
  } else {
    noTone(pinBzr);
    Blynk.virtualWrite(V4, LOW);
  }
}

void RGB(int merah, int hijau, int biru) {
  digitalWrite(pinMerah, merah);
  digitalWrite(pinHijau, hijau);
  digitalWrite(pinBiru, biru);
}

float hitungPPM(int adc) {
  float VRL = (adc / 4095.0) * 5.0;
  float RS = (5.0 - VRL) / VRL * RL;
  float rasio = RS / R0;
  float ppm = pow(10, ((log10(rasio) - kurvaCO2[1]) / kurvaCO2[2]) + kurvaCO2[0]);
  return ppm;
}

BLYNK_WRITE(V6) {
  controlfan = param.asInt();
  if (controlfan) {
    digitalWrite(pinRly, LOW);
    Blynk.virtualWrite(V5, LOW);
  } else {
    digitalWrite(pinRly, HIGH);
    Blynk.virtualWrite(V5, HIGH);
  }
}
