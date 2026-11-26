#include <stdint.h>

/* Indexed by the nearest coolant-temperature bin. */
__code const uint8_t starter_max_seconds[39] = {
     30,  30,  30,  30,  30,  30,  30,  25,  25,  25,  20,  20,  20,
     15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,
     15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,
};
