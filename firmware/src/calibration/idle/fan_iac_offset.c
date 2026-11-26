#include <stdint.h>

/* IAC compensation steps and delay counter limit for the fan. */
volatile __code const uint8_t fan_iac_offset = 8;
volatile __code const uint8_t fan_start_delay_ticks = 25;
