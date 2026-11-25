#include <stdint.h>

/* Adaptation offsets in steps and period in 20 ms ticks. */
volatile __code const int8_t iac_adaptation_min_offset = -15;
volatile __code const int8_t iac_adaptation_max_offset = 0;
volatile __code const uint8_t iac_adaptation_period = 100 / 20;
