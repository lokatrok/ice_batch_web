#ifndef PINS_H
#define PINS_H

// ======== SENSOR INPUTS ========
#define TEMP_SENSOR_PIN     32  // DS18B20 (OneWire)
#define TDS_SENSOR_PIN      34  // Analog input
#define FLOAT_SENSOR_PIN    5   // Float switch (HIGH = penuh)
#define FLOW_SENSOR_PIN     14  // YF-S201 / FS300A (pulse input)
#define FLOW_SWITCH_PIN     35  // Flow switch (HIGH = aliran OK)
#define COUNTDOWN_BUTTON    23  // Digital input (pull-up)

// ======== ACTUATOR OUTPUTS ========
#define VALVE_DRAIN_PIN     33  // Relay
#define VALVE_INLET_PIN     26  // Relay
#define COMPRESSOR_PIN      25  // Relay
#define PUMP_UV_PIN         27  // Relay
#define BUZZER_PIN          18  // Digital output
#define COUNTDOWN_LED       19  // Digital output

// ======== COMMUNICATION PINS ========
#define RTC_SDA_PIN         21  // I2C
#define RTC_SCL_PIN         22  // I2C

// Pin komunikasi Nextion (hanya untuk referensi, tidak digunakan di web)
// NEXTION_RX: 16 (Serial2)
// NEXTION_TX: 17 (Serial2)

#endif