#include <stdint.h>

/* Inclusive lower bound for the immediate high-sample shutdown. */
volatile __code const uint8_t oxygen_heater_high_adc_limit = 45;
