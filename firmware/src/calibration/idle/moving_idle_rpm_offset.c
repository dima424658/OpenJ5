#include <stdint.h>

/* Each raw unit corresponds to 10 RPM. */
volatile __code const uint8_t moving_idle_rpm_offset = 100 / 10;
