#include <stdint.h>

/* Half-bin RPM breakpoints. The final slot repeats the upper endpoint. */
__code const uint16_t main_rpm_half_bin_breakpoints[32] = {
     600,  660,  720,  780,  840,  900,  990, 1080,
    1170, 1260, 1380, 1500, 1650, 1800, 1950, 2130,
    2310, 2520, 2730, 3034, 3210, 3540, 3840, 4170,
    4530, 4920, 5370, 5850, 6360, 6930, 7500, 7500,
};

/* Percent breakpoints for the main throttle axis. */
__code const uint8_t main_throttle_breakpoints[16] = {
      0,   2,   4,   6,   8,  11,  14,  18,  23,  29,  37,  46,
     55,  65,  80, 100,
};
