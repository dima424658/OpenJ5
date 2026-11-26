#include <stdint.h>

/* Upper bound followed by lower bound, both in little-endian order. */
__code const uint8_t misfire_word_limits[4] = {
    230, 128,  27, 127,
};
