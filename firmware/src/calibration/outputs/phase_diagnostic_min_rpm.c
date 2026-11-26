#include <stdint.h>

/* Threshold above which the phase-sensor fault can be reported. */
volatile __code const uint8_t phase_diagnostic_min_rpm = 75;
