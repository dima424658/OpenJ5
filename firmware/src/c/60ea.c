#include <stdint.h>

/* Absolute fuel-cut threshold, in 30 RPM steps. */
volatile __code const uint8_t fuel_cut_rpm_threshold = 1590 / 30;
