#include <stdint.h>

/* Fractional ALF increment when moving toward a leaner mixture. */
volatile __code const uint8_t mixture_leaning_step_q8 = 33;
