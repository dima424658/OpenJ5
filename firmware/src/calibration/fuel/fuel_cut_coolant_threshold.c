#include <stdint.h>

/* Enable deceleration fuel cut from 40 deg C; raw coolant is T + 40. */
volatile __code const uint8_t fuel_cut_coolant_threshold = 40 + 40;
