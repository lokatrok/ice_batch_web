#ifndef DIGITAL_CONTROL_H
#define DIGITAL_CONTROL_H

#include <Arduino.h>

// Inisialisasi pin-pin digital
void initDigitalPins();

// Fungsi untuk mengatur output HIGH/LOW
void setBuzzer(bool state);
void setCountdownLED(bool state);
void setValveDrain(bool state);
void setValveInlet(bool state);
void setPumpUV(bool state);
void setCompressor(bool state);

// Fungsi untuk membaca input
bool isCountdownButtonPressed();
bool isFloatSensorLow(); // HIGH = penuh, LOW = kosong
bool isFlowSwitchOn();   // HIGH = aliran OK

// Fungsi umum (opsional)
void setPinOutput(int pin, bool state);
bool readPinInput(int pin);

#endif