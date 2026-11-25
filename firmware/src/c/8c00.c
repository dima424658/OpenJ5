#include <stdint.h>

/* Post-start position offsets and step intervals versus coolant temperature. */
__code const int8_t poststart_iac_upper_offset_8c00[39] = {
    17, 17, 17, 17, 17, 11,  8,  6,  5,  5,  5,  5,  5,
     5,  6,  7,  8,  9, 10, 11, 12, 12, 12, 12, 12, 12,
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
};

__code const int8_t poststart_iac_lower_offset_8c27[39] = {
     0,  0,  0,  0, -1, -2, -3, -3, -4, -4, -4, -4, -4,
    -4, -4, -4, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3,
    -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3,
};

__code const uint8_t poststart_iac_period_8c4e[39] = {
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
};
