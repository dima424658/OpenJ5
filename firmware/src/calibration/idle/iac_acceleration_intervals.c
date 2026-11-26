#include <stdint.h>

/* Step interval indexed by the absolute acceleration step. */
__code const uint8_t iac_acceleration_intervals[5] = {
     15,  15,  13,  11,   9,
};
