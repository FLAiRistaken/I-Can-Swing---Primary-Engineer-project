// include/Debug.h
#pragma once

#include <Arduino.h>

// Set to 1 to enable debugging, 0 to disable
#define DEBUG_ENABLED 1

#if DEBUG_ENABLED
    #define DEBUG_PRINT(x) Serial.print("DEBUG: "); Serial.print(x)
    #define DEBUG_PRINTLN(x) Serial.print("DEBUG: "); Serial.println(x)
    #define DEBUG_PRINT_VAR(name, value) Serial.print("DEBUG: "); Serial.print(name); Serial.print(": "); Serial.print(value)
    #define DEBUG_PRINTLN_VAR(name, value) Serial.print("DEBUG: "); Serial.print(name); Serial.print(": "); Serial.println(value)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINT_VAR(name, value)
    #define DEBUG_PRINTLN_VAR(name, value)
#endif
