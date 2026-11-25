#include <stdint.h>

/* Indexed by the nearest coolant-temperature bin. */
__code const uint8_t starter_cutoff_delay_cycles[39] = {
    100, 100, 100, 100, 100, 100, 100,  75,  75,  75,  75,  50,  50,
     50,  50,  50,  50,  50,  50,  50,  50,  50,  50,  50,  50,  50,
     50,  50,  50,  50,  50, 100, 100, 100, 100, 100, 100, 100, 100,
};
