#include <stdint.h>

enum { FUEL_HISTORY_BYTES = 8 };

static volatile __xdata __at (0xf839) uint8_t fuel_history_f839[FUEL_HISTORY_BYTES];

void clear_fuel_history_f839(void)
{
    volatile __xdata uint8_t *next = fuel_history_f839;
    uint8_t remaining = FUEL_HISTORY_BYTES;
    do {
        *next++ = 0;
    } while (--remaining != 0);
}
