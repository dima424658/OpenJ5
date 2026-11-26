#include <stdint.h>

/* The delay uses 20 ms ticks. The voltage threshold is an ADC byte. */
volatile __code const uint8_t shutdown_delay_raw = 120;
volatile __code const uint8_t shutdown_voltage_raw = 26;
