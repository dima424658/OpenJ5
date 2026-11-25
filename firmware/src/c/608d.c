#include <stdint.h>

/* Delay in 20 ms ticks. */
volatile __code const uint8_t fuel_cut_delay_ticks = 500 / 20;
