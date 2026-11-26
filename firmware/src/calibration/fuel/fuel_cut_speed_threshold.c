#include <stdint.h>

/* Minimum measured speed in km/h for the RPM-relative cut thresholds. */
volatile __code const uint8_t fuel_cut_speed_threshold = 8;
