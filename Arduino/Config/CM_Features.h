#ifndef CM_FEATURES_H
#define CM_FEATURES_H

// Compile-time hardware and service configuration for Arduino Uno.
// Use 1 to enable a feature and 0 to remove it from the build.

#define CM_FEATURE_LCD1602          1
#define CM_FEATURE_KEYPAD_4X4       1
#define CM_FEATURE_EXTERNAL_START   1
#define CM_FEATURE_HALL_SENSOR      1
#define CM_FEATURE_SSR              1
#define CM_FEATURE_BUZZER           1
#define CM_FEATURE_ESP32_UART       1
// Verbose human-readable Serial diagnostics are disabled in the production Uno
// image to preserve ATmega328P flash. CMP1 transport, safety, calibration,
// physical START and SSR behavior do not depend on this flag.
#ifndef CM_FEATURE_DIAGNOSTICS
#define CM_FEATURE_DIAGNOSTICS      0
#endif
#define CM_FEATURE_CALIBRATION      1

// Simulation is disabled in the normal machine firmware.
// When enabled, the real SSR must remain blocked.
#define CM_FEATURE_SIMULATION       0

#if CM_FEATURE_SIMULATION && CM_FEATURE_SSR
#define CM_SIMULATION_BLOCK_REAL_SSR 1
#else
#define CM_SIMULATION_BLOCK_REAL_SSR 0
#endif

#endif // CM_FEATURES_H
