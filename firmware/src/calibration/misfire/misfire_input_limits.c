#include <stdint.h>

/* Limits for RPM increase, the sample at F94A and elapsed seconds. */
__code const uint8_t misfire_input_limits[3] = {
     32,   5,  10,
};
