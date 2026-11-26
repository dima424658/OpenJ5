#include <stdint.h>

/* Raw post-start fuel decay factor, in units of 1/45 mg per cycle. */
volatile __code const uint8_t poststart_fuel_decay_gain = 45;
