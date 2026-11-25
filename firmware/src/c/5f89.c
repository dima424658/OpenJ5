#include <stdint.h>

/* Heater supply-voltage limit. The 0738 map labels it as the A/C cutoff. */
volatile __code const uint8_t oxygen_heater_supply_cutoff = 182;
