#include <stdint.h>

/* Raw DFREQ boundaries and adaptive idle-speed trim in units of 10 RPM. */
volatile __code const uint8_t idle_error_spread_low = 36;
volatile __code const uint8_t idle_error_spread_high = 144;
volatile __code const int8_t idle_rpm_trim_min = -10;
volatile __code const uint8_t idle_rpm_trim_max = 20;
