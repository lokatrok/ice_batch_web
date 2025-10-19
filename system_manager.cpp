#include "system_manager.h"
#include "digital_control.h"
#include "sensor_reader.h"
#include <Arduino.h>

// ==================== DEKLARASI VARIABEL GLOBAL (INSTANCE STRUCT) ====================
// Ini adalah variabel yang menyimpan state sistem
FillingState fillingState;
DrainingState drainingState;
CoolingState coolingState;
CirculationState circulationState;
WaterChangeState waterChangeState;
PrefillState prefillState;

// Konstanta untuk sistem (bisa disesuaikan)
const float FLOW_RATE_THRESHOLD = 0.1; // L/min, di bawah ini dianggap tidak ada aliran untuk draining
const int FILLING_FLOAT_DEBOUNCE_MS = 500; // Waktu untuk menghindari false trigger saat float sensor penuh
const float TEMP_HYSTERESIS = 2.0; // Histeresis untuk kontrol suhu (2 derajat)

// ==================== IMPLEMENTASI FUNGSI UTAMA ====================

void initSystem() {
  // Inisialisasi semua state ke nilai awal
  fillingState = FillingState();
  drainingState = DrainingState();
  coolingState = CoolingState();
  circulationState = CirculationState();
  waterChangeState = WaterChangeState();
  prefillState = PrefillState();

  Serial.println("System manager initialized.");
}

void tick() {
  // Update sensor dulu (jika perlu di setiap tick, bisa disesuaikan intervalnya)
  readSensors();

  // Jalankan proses aktif satu per satu, hanya jika tidak dalam error state (untuk sekarang)
  // Kita prioritaskan filling, draining, cooling manual terlebih dahulu
  if (fillingState.active) {
    runFillingProcess();
  } else if (drainingState.active) {
    runDrainingProcess();
  } else if (coolingState.active) {
    runCoolingProcess();
  }
  // Circulation, WaterChange, Prefill bisa ditambahkan nanti atau diprioritaskan berbeda

  // Jalankan scheduler otomatis dengan interval (misalnya 1x per detik)
  static unsigned long lastScheduleCheck = 0;
  if (millis() - lastScheduleCheck >= 1000) {
    // TODO: runAutoSchedule(); // Akan diimplementasikan nanti
    lastScheduleCheck = millis();
  }

  // Jalankan safety check dengan interval (misalnya 1x per detik)
  static unsigned long lastSafetyCheck = 0;
  if (millis() - lastSafetyCheck >= 1000) {
    // TODO: runCirculationSafety(); // Akan diimplementasikan nanti
    lastSafetyCheck = millis();
  }
}

void requestProcess(PROCESS_TYPE type, bool start) {
  if (start) {
    if (canStartProcess(type)) {
      switch(type) {
        case PROCESS_FILLING:
          fillingState.active = true;
          fillingState.stage = 0; // Reset ke stage awal
          fillingState.error = ProcessError(); // Reset error
          fillingState.stoppedBySensor = false;
          Serial.println("Filling process started.");
          break;
        case PROCESS_DRAINING:
          drainingState.active = true;
          drainingState.error = ProcessError(); // Reset error
          Serial.println("Draining process started.");
          break;
        case PROCESS_COOLING:
          coolingState.active = true;
          coolingState.error = ProcessError(); // Reset error
          coolingState.initialCoolingMode = true; // Reset ke mode awal
          coolingState.targetReached = false;
          Serial.println("Cooling process started.");
          break;
        case PROCESS_CIRCULATION:
          // TODO: Implementasi nanti
          break;
        case PROCESS_WATER_CHANGE:
          // TODO: Implementasi nanti
          break;
        case PROCESS_PREFILL:
          // TODO: Implementasi nanti
          break;
      }
    } else {
      Serial.println("Cannot start process: conflict detected.");
      // Bisa kirim error ke web nanti
    }
  } else { // Stop process
    switch(type) {
      case PROCESS_FILLING:
        fillingState.active = false;
        fillingState.stage = 0;
        setValveInlet(false);
        setValveDrain(false);
        Serial.println("Filling process stopped.");
        break;
      case PROCESS_DRAINING:
        drainingState.active = false;
        setValveDrain(false);
        setPumpUV(false);
        Serial.println("Draining process stopped.");
        break;
      case PROCESS_COOLING:
        coolingState.active = false;
        setCompressor(false);
        setPumpUV(false);
        Serial.println("Cooling process stopped.");
        break;
      case PROCESS_CIRCULATION:
        // TODO: Implementasi nanti
        break;
      case PROCESS_WATER_CHANGE:
        // TODO: Implementasi nanti
        break;
      case PROCESS_PREFILL:
        // TODO: Implementasi nanti
        break;
    }
  }
}

bool canStartProcess(PROCESS_TYPE type) {
  // Logika konflik: proses tidak bisa jalan jika proses lain yang bentrok sedang aktif
  // Prioritaskan filling, draining, cooling manual dulu
  switch(type) {
    case PROCESS_FILLING:
      return !(drainingState.active || circulationState.active || waterChangeState.active);
    case PROCESS_DRAINING:
      return !(fillingState.active || coolingState.active || circulationState.active || waterChangeState.active);
    case PROCESS_COOLING:
      return !(fillingState.active || drainingState.active || circulationState.active || waterChangeState.active);
    case PROCESS_CIRCULATION:
      // Circulation bisa bentrok dengan filling, draining, cooling manual
      return !(fillingState.active || drainingState.active || coolingState.active);
    case PROCESS_WATER_CHANGE:
      // Water change bisa bentrok dengan filling, draining, cooling, circulation, prefill
      return !(fillingState.active || drainingState.active || coolingState.active || circulationState.active || prefillState.active);
    case PROCESS_PREFILL:
      // Prefill bisa bentrok dengan filling, draining, cooling, circulation, water change
      return !(fillingState.active || drainingState.active || coolingState.active || circulationState.active || waterChangeState.active);
    default:
      return false;
  }
}

// ==================== IMPLEMENTASI FUNGSI PROSES INTI ====================

void runFillingProcess() {
  if (!fillingState.active) return;

  // RECOVERY LOGIC: Cek apakah error bisa direcovery sebelum mengecek error aktif
  if (fillingState.error.active) {
    // Contoh recovery: jika error adalah FLOW_SWITCH_OFF dan sekarang OK
    if (fillingState.error.code == FLOW_SWITCH_OFF) {
      if (isFlowSwitchOn()) { // Jika flow switch sekarang menyala
        clearError(fillingState.error); // Clear error
        Serial.println("Filling: Error recovered - Flow switch is back ON.");
        // Jangan return di sini, lanjutkan ke logika filling normal
        // Proses akan melanjutkan dari stage 2 jika memang sedang di stage 2
      }
    }
  }

  // Jika error masih aktif setelah recovery check, hentikan proses
  if (fillingState.error.active) {
    setValveInlet(false);
    setValveDrain(false);
    fillingState.active = false;
    Serial.println("Filling stopped due to unrecovered error.");
    return;
  }

  unsigned long now = millis();

  switch (fillingState.stage) {
    case 0: // Cek kondisi awal
        {
            bool floatState = isFloatSensorLow(); // LOW = penuh, HIGH = kosong/rendah

            // Jika air sudah penuh, hentikan proses
            if (!floatState) { // Float HIGH = air rendah, LOW = air penuh
                requestProcess(PROCESS_FILLING, false); // Stop filling
                return;
            }

            // Jika air rendah, mulai draining awal 5 detik (untuk reset)
            setValveDrain(false); // Pastikan drain tertutup
            setValveInlet(false); // Pastikan inlet tertutup
            delay(100); // Tunggu sebentar

            setValveDrain(true); // Buka valve drain
            fillingState.drainStartTime = now;
            fillingState.stage = 1;
            Serial.println("Filling: Stage 1 - Draining first 5s.");
        }
        break;

    case 1: // Draining awal 5 detik
        if (now - fillingState.drainStartTime >= 5000) {
            setValveDrain(false); // Tutup valve drain
            delay(500); // Tunggu sebentar agar stabil

            setValveInlet(true); // Buka valve inlet
            fillingState.stage = 2; // Pindah ke filling aktif
            Serial.println("Filling: Stage 2 - Filling started.");
        }
        break;

    case 2: // Filling aktif
        {
            bool floatState = isFloatSensorLow(); // LOW = penuh
            bool flowSwitchState = isFlowSwitchOn(); // HIGH = aliran OK

            // Cek error: jika flow switch mati (tidak ada aliran saat valve inlet terbuka)
            if (!flowSwitchState) { // Flow switch OFF saat filling
                setError(fillingState.error, FLOW_SWITCH_OFF, "Flow switch off during filling");
                setValveInlet(false); // Matikan valve inlet
                fillingState.active = false; // Hentikan proses
                Serial.println("Filling: Error - Flow switch off.");
                return;
            }

            // Cek apakah air sudah penuh
            if (!floatState) { // Float sensor LOW = penuh
                // Gunakan debounce untuk mencegah false trigger
                if (fillingState.fullDetectedTime == 0) {
                    fillingState.fullDetectedTime = now; // Catat waktu pertama kali penuh
                } else if (now - fillingState.fullDetectedTime >= FILLING_FLOAT_DEBOUNCE_MS) {
                    setValveInlet(false); // Matikan valve inlet
                    fillingState.active = false; // Hentikan proses
                    fillingState.stage = 0; // Reset stage
                    fillingState.fullDetectedTime = 0; // Reset debounce timer
                    Serial.println("Filling: Completed - Tank full.");
                    return;
                }
                // Jika belum melewati debounce, tetap lanjutkan filling
            } else {
                fillingState.fullDetectedTime = 0; // Reset debounce jika air tidak penuh
            }
        }
        break;
  }
}

void runDrainingProcess() {
  if (!drainingState.active) return;

  // Jika error aktif, hentikan proses
  if (drainingState.error.active) {
    setValveDrain(false);
    setPumpUV(false);
    drainingState.active = false;
    Serial.println("Draining stopped due to error.");
    return;
  }

  // Cek laju aliran dari sensor_reader
  float currentFlowRate = getCurrentFlowRate();

  // Jika laju aliran di bawah ambang batas, hentikan draining
  if (currentFlowRate < FLOW_RATE_THRESHOLD) {
      setValveDrain(false);
      setPumpUV(false);
      drainingState.active = false;
      Serial.println("Draining: Stopped - Flow rate too low.");
      return;
  }

  // Jika laju aliran OK, pastikan valve dan pump menyala
  setValveDrain(true);
  setPumpUV(true); // Pompa UV dinyalakan selama draining (untuk sirkulasi air ke drain)

  // (Opsional) Cek float sensor sebagai backup jika flow sensor gagal
  // bool floatState = isFloatSensorLow(); // HIGH = kosong
  // if (floatState) { // Jika air sudah kosong
  //     setValveDrain(false);
  //     setPumpUV(false);
  //     drainingState.active = false;
  //     Serial.println("Draining: Stopped - Tank empty (backup check).");
  //     return;
  // }
}

void runCoolingProcess() {
  if (!coolingState.active) return;

  // Jika error aktif, hentikan proses
  if (coolingState.error.active) {
    setCompressor(false);
    setPumpUV(false);
    coolingState.active = false;
    Serial.println("Cooling stopped due to error.");
    return;
  }

  // Baca suhu dari sensor_reader
  float currentTemp = getCurrentTemperature();
  // Baca target suhu dari variabel global (misalnya dari setting manual atau auto)
  // int targetTemp = suhuTarget; // Ambil dari variabel global atau getter
  // Untuk sekarang, gunakan contoh target
  int targetTemp = 15; // Ganti dengan getter nanti

  // Jika suhu gagal dibaca, hentikan proses
  if (currentTemp == -99.0) { // Kode error dari sensor_reader
      setError(coolingState.error, SENSOR_READ_FAILED, "Temperature sensor read failed");
      setCompressor(false);
      setPumpUV(false);
      coolingState.active = false;
      Serial.println("Cooling: Error - Temperature sensor failed.");
      return;
  }

  // Selalu nyalakan pompa UV selama cooling aktif
  setPumpUV(true);

  // Logika kontrol kompresor
  if (coolingState.initialCoolingMode) {
      // Mode awal: matikan kompresor saat target pertama kali tercapai
      if (currentTemp <= targetTemp) {
          setCompressor(false);
          coolingState.initialCoolingMode = false; // Pindah ke mode histeresis
          coolingState.targetReached = true;
          Serial.println("Cooling: Target reached, switching to hysteresis mode.");
      } else {
          setCompressor(true); // Nyalakan kompresor jika belum sampai target
      }
  } else {
      // Mode histeresis: nyalakan jika suhu naik melebihi target + histeresis
      if (currentTemp > (targetTemp + TEMP_HYSTERESIS)) {
          setCompressor(true);
          Serial.println("Cooling: Temp too high, compressor ON (hysteresis).");
      } else if (currentTemp <= targetTemp) {
          setCompressor(false);
          Serial.println("Cooling: Temp OK, compressor OFF (hysteresis).");
      }
      // Jika suhu di antara target dan target+histeresis, biarkan kompresor sesuai state sebelumnya
  }
}

// ==================== IMPLEMENTASI FUNGSI ERROR (Sederhana - Fase 1) ====================

void setError(ProcessError& errorRef, int code, String message) {
  errorRef.active = true;
  errorRef.startTime = millis();
  errorRef.code = code;
  errorRef.message = message;
  Serial.println("Error [" + String(code) + "]: " + message);
}

void clearError(ProcessError& errorRef) {
  errorRef.active = false;
  errorRef.code = NO_ERROR;
  errorRef.message = "";
  Serial.println("Error cleared.");
}

bool canRecoverError(int errorCode) {
  // Untuk sekarang, kita tidak recovery otomatis
  // Fungsi ini bisa digunakan di Fase 2
  return false;
}

// ==================== IMPLEMENTASI GETTER ====================

bool isFillingActive() { return fillingState.active; }
bool isDrainingActive() { return drainingState.active; }
bool isCoolingActive() { return coolingState.active; }
bool isCirculationActive() { return circulationState.active; }
bool isWaterChangeActive() { return waterChangeState.active; }
bool isPrefillActive() { return prefillState.active; }