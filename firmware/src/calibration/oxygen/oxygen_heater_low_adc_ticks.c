#include <stdint.h>

/* Consecutive low heater ADC samples required to set the inhibit latch. */
volatile __code const uint8_t oxygen_heater_low_adc_ticks = 151;
