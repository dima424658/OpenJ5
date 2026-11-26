#include <stdint.h>

/* Minimum vehicle speed to engage A/C outside the idle mode. */
volatile __code const uint8_t ac_engage_speed_threshold = 12;
