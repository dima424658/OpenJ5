#include <stdint.h>

/* Consecutive low ignition-supply samples before setting the shutdown flag. */
volatile __code const uint8_t ignition_supply_low_samples = 6;
