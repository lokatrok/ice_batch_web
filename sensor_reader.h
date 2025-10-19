#ifndef SENSOR_READER_H
#define SENSOR_READER_H

#include <Arduino.h>

// Inisialisasi sensor
void initSensors();

// Fungsi baca sensor utama
void readSensors();

// Fungsi untuk mendapatkan data sensor dalam format JSON
String getSensorDataJSON();

// Getter untuk masing-masing sensor
float getCurrentTemperature();
float getCurrentFlowRate();
float getCurrentTDS();

// Getter untuk RTC
String getRTCTime(); // Format: "HH:MM"
String getRTCDate(); // Format: "DD/MM/YYYY"
bool isRTCValid();   // Cek apakah RTC menyimpan waktu yang valid

#endif