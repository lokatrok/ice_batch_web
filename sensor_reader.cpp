#include "sensor_reader.h"
#include "digital_control.h" // <-- Tambahkan ini untuk mengakses fungsi dari digital_control
#include "pins.h"
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h> // <-- Tambahkan untuk I2C
#include <RTClib.h> // <-- Tambahkan library RTC

// Objek RTC
RTC_DS3231 rtc;

// Konstanta dari kode lama
const float FS300A_CALIBRATION = 660.0; // Pulses per liter untuk FS300A
const int FLOW_MAX_RATE = 60;          // Maksimal 60 L/min
const int FLOW_SAMPLES = 5;            // Jumlah sampel untuk filter
const int DEBOUNCE_TIME = 2;           // 2ms debounce time

// Objek sensor
OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature sensors(&oneWire);

// Variabel global untuk menyimpan nilai sensor
float currentTemp = 0.0;
float currentFlowRate = 0.0;
float currentTDS = 0.0;

// Variabel RTC
bool rtcValid = false;

// Variabel flow sensor
volatile unsigned long pulseCount = 0;
unsigned long lastPulseCount = 0;
unsigned long lastFlowCalcTime = 0;
float flowReadings[FLOW_SAMPLES];    // Array untuk menyimpan sampel flow rate
int flowIndex = 0;                   // Indeks untuk array
unsigned long lastInterruptTime = 0; // Untuk debounce interrupt

// Fungsi interrupt
void IRAM_ATTR flowISR() {
  unsigned long now = micros();
  if (now - lastInterruptTime > DEBOUNCE_TIME * 1000) {
    pulseCount++;
    lastInterruptTime = now;
  }
}

void initSensors() {
  // Inisialisasi pin sensor digital dilakukan di digital_control.cpp
  // Kita asumsikan initDigitalPins() dipanggil sebelum initSensors()

  // Inisialisasi sensor suhu
  sensors.begin();

  // Inisialisasi RTC
  Wire.begin(RTC_SDA_PIN, RTC_SCL_PIN); // Gunakan pin dari pins.h
  if (!rtc.begin()) {
    Serial.println("Tidak dapat menemukan RTC!");
    rtcValid = false;
  } else {
    if (rtc.lostPower()) {
      Serial.println("RTC kehilangan daya, atur waktu!");
      // Anda bisa menambahkan logika untuk mengatur waktu jika perlu
      // rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Atur ke waktu compile
      rtcValid = false; // Tandai sebagai tidak valid jika kehilangan daya
    } else {
      rtcValid = true;
      Serial.println("RTC initialized and valid.");
    }
  }

  // Inisialisasi array filter flow rate
  for (int i = 0; i < FLOW_SAMPLES; i++) {
    flowReadings[i] = 0;
  }

  // Inisialisasi interrupt flow sensor
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), flowISR, RISING);

  Serial.println("Sensors initialized.");
}

void readSensors() {
  // ================= TEMPERATURE SENSOR =================
  sensors.requestTemperatures();
  currentTemp = sensors.getTempCByIndex(0);

  // Handle DS18B20 error codes
  if (currentTemp == -127.00 || currentTemp == 85.00) {
    currentTemp = -99.0; // Error indicator
  }

  // ================= FLOW SENSOR (FS300A) =================
  unsigned long currentTime = millis();
  if (currentTime - lastFlowCalcTime >= 500) {  // Update setiap 500ms
    noInterrupts();
    unsigned long pulses = pulseCount - lastPulseCount;
    lastPulseCount = pulseCount;
    interrupts();

    if (pulses == 0) {
      currentFlowRate = 0.0;
    } else {
      currentFlowRate = (pulses * 60.0) / FS300A_CALIBRATION;
      if (currentFlowRate > FLOW_MAX_RATE) currentFlowRate = FLOW_MAX_RATE;
    }

    lastFlowCalcTime = currentTime;
  }

  // ================= TDS SENSOR =================
  int raw = analogRead(TDS_SENSOR_PIN);
  float voltage = raw * (3.3 / 4095.0);

  // Validate TDS sensor
  if (raw < 100 || raw > 4000 || voltage < 0.1 || voltage > 3.2) {
    currentTDS = -1; // Error indicator
  } else {
    // Calculate EC (Electrical Conductivity)
    float ec = (133.42 * voltage * voltage * voltage
               - 255.86 * voltage * voltage
               + 857.39 * voltage);
    if (ec < 0) ec = 0;
    if (ec > 3000) ec = 3000;
    currentTDS = ec * 0.5; // Convert to PPM
    if (currentTDS > 9999) currentTDS = 9999;
  }

  // NOTE: Pembacaan input digital (float, flow switch) tetap dilakukan
  // di digital_control.cpp. Jika ingin disimpan di sini, Anda bisa
  // memanggil fungsi dari digital_control.cpp di sini.
  // Contoh:
  // bool floatState = isFloatSensorLow(); // <-- Ini dari digital_control.cpp
  // bool flowSwitchState = isFlowSwitchOn(); // <-- Ini dari digital_control.cpp
}

String getSensorDataJSON() {
  readSensors(); // Pastikan data terbaru

  // Kita ambil nilai input digital dari digital_control
  bool floatState = isFloatSensorLow();
  bool flowSwitchState = isFlowSwitchOn();

  String json = "{";
  json += "\"temp\":" + String(currentTemp, 2);
  json += ",\"flowRate\":" + String(currentFlowRate, 2);
  json += ",\"tds\":" + String(currentTDS, 0);
  json += ",\"float\":" + String(floatState);
  json += ",\"flowSwitch\":" + String(flowSwitchState);
  json += ",\"time\":\"" + getRTCTime() + "\""; // Tambahkan waktu
  json += ",\"date\":\"" + getRTCDate() + "\""; // Tambahkan tanggal
  json += ",\"rtcValid\":" + String(isRTCValid() ? "true" : "false"); // Tambahkan status RTC
  json += "}";
  return json;
}

// Getter functions
float getCurrentTemperature() { return currentTemp; }
float getCurrentFlowRate() { return currentFlowRate; }
float getCurrentTDS() { return currentTDS; }

// Getter functions untuk RTC
String getRTCTime() {
  if (!rtcValid) return "ERR";
  DateTime now = rtc.now();
  char timeStr[6];
  sprintf(timeStr, "%02d:%02d", now.hour(), now.minute());
  return String(timeStr);
}

String getRTCDate() {
  if (!rtcValid) return "ERR";
  DateTime now = rtc.now();
  char dateStr[11];
  sprintf(dateStr, "%02d/%02d/%04d", now.day(), now.month(), now.year());
  return String(dateStr);
}

bool isRTCValid() {
  return rtcValid;
}