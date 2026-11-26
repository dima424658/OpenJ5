#include <stdint.h>

enum { IAC_HISTORY_BYTES = 10 };

static volatile __xdata __at (0xf851) uint8_t iac_history[IAC_HISTORY_BYTES];

void clear_iac_history(void)
{
    volatile __xdata uint8_t *next = iac_history;
    uint8_t remaining = IAC_HISTORY_BYTES;
    do {
        *next++ = 0;
    } while (--remaining != 0);
}
