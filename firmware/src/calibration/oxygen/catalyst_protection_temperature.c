#include <stdint.h>

/* Exhaust temperature for catalyst enrichment, in 8 °C units (824 °C). */
volatile __code const uint8_t catalyst_protection_temperature = 103;
