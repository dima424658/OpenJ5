#include <stdint.h>

/* Injection phase delay correction at the reference RPM. */
volatile __code const uint8_t injection_phase_delay = 8;
