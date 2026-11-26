#include <stdint.h>

/* Heater pause in scheduler ticks, indexed by the upper nibble of battery voltage. */
__code const uint8_t oxygen_heater_pause_curve[16] = {
    0, 0, 0, 0, 0, 0, 1, 2,
    3, 4, 5, 6, 8, 10, 10, 10,
};
