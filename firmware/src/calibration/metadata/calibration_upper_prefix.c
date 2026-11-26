#include <stdint.h>

/* The first mapped table in the upper calibration region starts at 0x8193. */
__code const uint8_t calibration_upper_prefix[0x8193 - 0x8000] = { 0 };
