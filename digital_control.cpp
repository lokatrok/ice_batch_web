#include "digital_control.h"
#include "pins.h" // <-- Penting! Agar bisa mengakses COUNTDOWN_BUTTON, BUZZER_PIN, dll
#include <Arduino.h>

void initDigitalPins() {
  // Inisialisasi pin INPUT
  pinMode(COUNTDOWN_BUTTON, INPUT_PULLUP);
  pinMode(FLOAT_SENSOR_PIN, INPUT_PULLUP);
  pinMode(FLOW_SWITCH_PIN, INPUT_PULLUP);

  // Inisialisasi pin OUTPUT
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(COUNTDOWN_LED, OUTPUT);
  pinMode(VALVE_DRAIN_PIN, OUTPUT);
  pinMode(VALVE_INLET_PIN, OUTPUT);
  pinMode(PUMP_UV_PIN, OUTPUT);
  pinMode(COMPRESSOR_PIN, OUTPUT);

  // Set awal semua output ke LOW (aktuator OFF)
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(COUNTDOWN_LED, LOW);
  digitalWrite(VALVE_DRAIN_PIN, LOW);
  digitalWrite(VALVE_INLET_PIN, LOW);
  digitalWrite(PUMP_UV_PIN, LOW);
  digitalWrite(COMPRESSOR_PIN, LOW);

  Serial.println("Digital pins initialized.");
}

// --- Fungsi untuk mengatur output ---
void setBuzzer(bool state) {
  digitalWrite(BUZZER_PIN, state ? HIGH : LOW);
}

void setCountdownLED(bool state) {
  digitalWrite(COUNTDOWN_LED, state ? HIGH : LOW);
}

void setValveDrain(bool state) {
  digitalWrite(VALVE_DRAIN_PIN, state ? HIGH : LOW);
}

void setValveInlet(bool state) {
  digitalWrite(VALVE_INLET_PIN, state ? HIGH : LOW);
}

void setPumpUV(bool state) {
  digitalWrite(PUMP_UV_PIN, state ? HIGH : LOW);
}

void setCompressor(bool state) {
  digitalWrite(COMPRESSOR_PIN, state ? HIGH : LOW);
}

// --- Fungsi untuk membaca input ---
bool isCountdownButtonPressed() {
  // INPUT_PULLUP: LOW = ditekan
  return digitalRead(COUNTDOWN_BUTTON) == LOW;
}

bool isFloatSensorLow() {
  // INPUT_PULLUP: LOW = air rendah/kosong
  return digitalRead(FLOAT_SENSOR_PIN) == HIGH;
}

bool isFlowSwitchOn() {
  // INPUT_PULLUP: HIGH = aliran OK
  return digitalRead(FLOW_SWITCH_PIN) == HIGH;
}

// --- Fungsi umum (opsional) ---
void setPinOutput(int pin, bool state) {
  digitalWrite(pin, state ? HIGH : LOW);
}

bool readPinInput(int pin) {
  return digitalRead(pin) == HIGH;
}