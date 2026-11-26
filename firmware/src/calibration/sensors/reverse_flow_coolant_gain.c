#include <stdint.h>

/* Temperature correction of the cylinder charge limit, Q7. */
__code const uint8_t reverse_flow_coolant_gain[39] = {
    230, 230, 227, 223, 219, 215, 211, 206, 201, 196, 189, 183, 176,
    169, 163, 156, 151, 146, 141, 137, 133, 131, 129, 128, 128, 128,
    128, 128, 128, 128, 128, 127, 125, 123, 120, 118, 116, 115, 115,
};
