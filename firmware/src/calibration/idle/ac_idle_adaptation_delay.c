#include <stdint.h>

/* Idle compensation delay in 20 ms ticks. */
volatile __code const uint8_t ac_idle_adaptation_delay = 25;
