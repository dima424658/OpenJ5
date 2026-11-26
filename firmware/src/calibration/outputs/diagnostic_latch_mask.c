#include <stdint.h>

/* Enable mask for latched diagnostic flags. */
volatile __code const uint8_t diagnostic_latch_mask = 253;
