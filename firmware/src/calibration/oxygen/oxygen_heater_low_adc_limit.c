#include <stdint.h>

/* Exclusive upper bound for the low-sample diagnostic branch. */
volatile __code const uint8_t oxygen_heater_low_adc_limit = 2;
