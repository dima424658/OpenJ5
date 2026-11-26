#include <stdint.h>

/* From -40 to 150 deg C in steps of 5 deg C; each value is in sixteenths. */
__code const uint8_t poststart_fuel_decay_by_temp[39] = {
    40, 39, 38, 37, 36, 35, 33, 31, 29, 27, 25, 23, 21,
    20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10,  9,  8,
     8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
};
