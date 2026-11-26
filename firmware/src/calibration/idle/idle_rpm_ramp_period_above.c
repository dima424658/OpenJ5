#include <stdint.h>

/* Compare the update counter against these raw values before dropping 10 RPM. */
volatile __code const uint8_t idle_rpm_ramp_period_above = 1;
volatile __code const uint8_t idle_rpm_ramp_period_below = 2;
