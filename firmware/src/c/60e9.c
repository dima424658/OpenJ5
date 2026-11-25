#include <stdint.h>

/* Absolute fuel-resume threshold, in 30 RPM steps. */
volatile __code const uint8_t fuel_resume_rpm_threshold = 1200 / 30;
