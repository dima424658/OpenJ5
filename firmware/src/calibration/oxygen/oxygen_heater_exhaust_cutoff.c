#include <stdint.h>

/* Exhaust temperature for heater cutoff, in 8 °C units (760 °C). */
volatile __code const uint8_t oxygen_heater_exhaust_cutoff = 95;
