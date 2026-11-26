#include <stdint.h>

/* Raw per-cycle decrement of the poststart fuel floor by coolant temperature. */
__code const uint8_t poststart_fuel_decay_by_coolant[39] = {
    90, 90, 90, 86, 80, 74, 68, 61, 51, 41, 30, 18, 15,
    13, 12, 11, 10, 9, 8, 7, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 18, 18, 18, 18, 18, 18, 18,
};
