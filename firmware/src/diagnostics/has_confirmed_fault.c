#include <stdint.h>

enum {
    FAULT_HISTORY_SIZE = 64,
    FAULT_CONFIRMED = 0x80,
};

static volatile __xdata __at (0xf512) uint8_t fault_history[FAULT_HISTORY_SIZE];

__bit has_confirmed_fault(void)
{
    for (uint8_t i = 0; i < FAULT_HISTORY_SIZE; ++i) {
        if (fault_history[i] & FAULT_CONFIRMED) {
            return 1;
        }
    }
    return 0;
}
