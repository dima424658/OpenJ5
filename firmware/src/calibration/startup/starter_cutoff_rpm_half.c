#include <stdint.h>

/* Indexed by the nearest coolant-temperature bin. */
__code const uint8_t starter_cutoff_rpm_half[39] = {
    120, 120, 120, 120, 120, 120, 110, 100, 100,  95,  95,  90,  90,
     85,  85,  80,  80,  75,  75,  75,  70,  70,  70,  70,  70,  70,
     70,  70,  70,  70,  70,  75,  80,  80,  80,  80,  80,  80,  80,
};
