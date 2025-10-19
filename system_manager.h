#ifndef SYSTEM_MANAGER_H
#define SYSTEM_MANAGER_H

#include <Arduino.h> // Kita butuh tipe data seperti bool, int, String, dll

// ==================== DEFINISI ERROR ====================
// Kode error umum untuk semua proses
enum ErrorCodes {
  NO_ERROR = 0,
  FLOW_SWITCH_OFF = 1,        // Flow switch mati saat filling/draining
  FLOAT_SENSOR_STUCK = 2,     // Float sensor tidak berubah selama waktu lama
  SENSOR_READ_FAILED = 3,     // Sensor gagal dibaca
  TIMEOUT_ERROR = 4,          // Proses melebihi batas waktu
  LOW_FLOW_ERROR = 5,         // Flow rate terlalu rendah
  // Tambahkan kode error lain sesuai kebutuhan
};

// ==================== STRUCT ERROR (Umum) ====================
struct ProcessError {
  bool active = false;              // Apakah error aktif
  unsigned long startTime = 0;      // Waktu error mulai
  int code = NO_ERROR;              // Kode error (dari enum ErrorCodes)
  String message = "";              // Pesan error opsional
};

// ==================== STRUCT PROSES ====================

// --- Filling ---
struct FillingState {
  bool active = false;
  int stage = 0; // 0: idle, 1: draining awal, 2: filling aktif
  ProcessError error; // <-- Pastikan ini ada
  bool stoppedBySensor = false;
  unsigned long drainStartTime = 0;      // Waktu mulai draining awal
  unsigned long fullDetectedTime = 0;    // Waktu saat sensor float mendeteksi penuh
  // Tambahkan variabel lain jika diperlukan
};

// --- Draining ---
struct DrainingState {
  bool active = false;
  ProcessError error; // <-- Pastikan ini ada
  // Tambahkan variabel lain jika diperlukan
};

// --- Cooling ---
struct CoolingState {
  bool active = false;
  bool sistemAktif = false; // Apakah pompa UV dan kontrol kompresor aktif
  bool initialCoolingMode = true; // Mode awal untuk mencapai target
  bool targetReached = false; // Apakah target suhu pertama kali tercapai
  unsigned long coolingStartTime = 0;
  unsigned long lastTempCheckTime = 0;
  ProcessError error; // <-- INI YANG KETINGGALAN, TAMBAHKAN BARIS INI
  // Tambahkan variabel lain jika diperlukan
};

// --- Circulation ---
struct CirculationState {
  bool active = false;
  int stage = 0; // 0: cek level, 1: cooling
  ProcessError error; // <-- Pastikan ini ada
  unsigned long startTime = 0;
  // Tambahkan variabel lain jika diperlukan
};

// --- Water Change ---
struct WaterChangeState {
  bool active = false;
  int stage = 0; // 0: draining, 1: filling, 2: error
  ProcessError error; // <-- Pastikan ini ada
  unsigned long drainStartTime = 0; // Untuk stage 0
  unsigned long fillStartTime = 0;  // Untuk stage 1
  unsigned long fullDetectedTime = 0; // Untuk stage 1
  // Tambahkan variabel lain jika diperlukan
};

// --- Prefill ---
struct PrefillState {
  bool active = false;
  int stage = 0; // 0: idle, 1: draining, 2: filling
  ProcessError error; // <-- Pastikan ini ada
  bool started = false;
  unsigned long drainStartTime = 0; // Untuk stage 1
  unsigned long fillStartTime = 0;  // Untuk stage 2
  unsigned long fullDetectedTime = 0; // Untuk stage 2
  // Tambahkan variabel lain jika diperlukan
};

// ==================== ENUM JENIS PROSES (Untuk mekanisme sentral) ====================
enum PROCESS_TYPE {
  PROCESS_FILLING,
  PROCESS_DRAINING,
  PROCESS_COOLING,
  PROCESS_CIRCULATION,
  PROCESS_WATER_CHANGE,
  PROCESS_PREFILL
};

// ==================== DEKLARASI FUNGSI UTAMA ====================

// Inisialisasi sistem
void initSystem();

// Fungsi utama yang dipanggil di loop()
void tick();

// Fungsi untuk meminta start/stop proses (mekanisme sentral)
void requestProcess(PROCESS_TYPE type, bool start);

// Fungsi untuk mengecek apakah proses bisa dimulai (mekanisme sentral)
bool canStartProcess(PROCESS_TYPE type);

// Fungsi untuk mengecek apakah proses sedang aktif (getter)
bool isFillingActive();
bool isDrainingActive();
bool isCoolingActive();
bool isCirculationActive();
bool isWaterChangeActive();
bool isPrefillActive();

// ==================== DEKLARASI FUNGSI PROSES (Internal) ====================

// Filling
void runFillingProcess();
void fillingState0_CheckInitial(unsigned long now);
void fillingState1_DrainFirst(unsigned long now);
void fillingState2_FillActive(unsigned long now);
void handleFillingErrorRecovery();

// Draining
void runDrainingProcess();
// ... tambahkan fungsi internal untuk draining

// Cooling
void runCoolingProcess();
// ... tambahkan fungsi internal untuk cooling

// Circulation
void runCirculationProcess();
// ... tambahkan fungsi internal untuk circulation

// Water Change
void runWaterChangeProcess();
// ... tambahkan fungsi internal untuk water change

// Prefill
void runPrefillProcess();
// ... tambahkan fungsi internal untuk prefill

// ==================== DEKLARASI FUNGSI ERROR (Internal) ====================

void setError(ProcessError& errorRef, int code, String message);
void clearError(ProcessError& errorRef);
bool canRecoverError(int errorCode);

#endif // SYSTEM_MANAGER_H