#include <stdint.h>

/* The controller multiplies error by each raw gain and divides by 32. */
volatile __code const uint8_t idle_ignition_gain_nonnegative = 64;
volatile __code const uint8_t idle_ignition_gain_negative = 64;

/* RPM error is in units of 10 RPM. */
volatile __code const uint8_t idle_ignition_deadband = 1;
