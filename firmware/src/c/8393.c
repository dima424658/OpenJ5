#include <stdint.h>

/* Charge extrapolation for throttle opening by coolant temperature. */
__code const uint8_t opening_charge_gain[39] = {
    64, 64, 62, 60, 58, 56, 54, 52, 50, 48, 46, 44, 42,
    40, 38, 36, 34, 32, 30, 28, 26, 24, 22, 20, 18, 17,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
};
