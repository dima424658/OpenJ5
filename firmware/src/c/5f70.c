#include <stdint.h>

/* Coolant threshold for running knock control, in the stock encoding. */
volatile __code const uint8_t knock_min_coolant = 90;
