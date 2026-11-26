#include <stdint.h>

/* Economy-mode throttle boundary by half-step RPM bin. */
__code const uint8_t economy_throttle_boundary[32] = {
     20,  20,  20,  20,  20,  20,  20,  20,  21,  22,  23,  24,  25,  26,  27,  28,
     30,  32,  34,  38,  40,  40,  40,  40,  40,  40,  40,  40,  40,  40,  40,  40,
};
