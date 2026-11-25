#include <stdint.h>

/* Coolant threshold in the stock temperature encoding (-5 °C). */
volatile __code const uint8_t oxygen_heater_coolant_cutoff = 35;
