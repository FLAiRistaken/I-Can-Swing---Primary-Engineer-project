// test/unity_config.h
#ifndef UNITY_CONFIG_H
#define UNITY_CONFIG_H

// Define empty output functions
#define UNITY_OUTPUT_START() do {} while(0)
#define UNITY_OUTPUT_CHAR(a) do {} while(0)
#define UNITY_OUTPUT_FLUSH() do {} while(0)
#define UNITY_OUTPUT_COMPLETE() do {} while(0)

// Avoid float issues
#define UNITY_EXCLUDE_FLOAT
#define UNITY_EXCLUDE_DOUBLE
#define UNITY_EXCLUDE_FLOAT_PRINT

// Avoid C++ complexities in C code
#define UNITY_EXCLUDE_STDINT_H
#define UNITY_EXCLUDE_SETJMP_H

#endif // UNITY_CONFIG_H
