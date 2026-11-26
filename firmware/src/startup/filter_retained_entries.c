#include <stdint.h>

enum { RETAINED_ENTRY_VALID = 0x80 };

static volatile __xdata __at (0xf4d2) uint8_t retained_entries[64];

void filter_retained_entries(void)
{
    volatile __xdata uint8_t *entry = retained_entries;
    uint8_t remaining = sizeof(retained_entries);
    do {
        if ((*entry & RETAINED_ENTRY_VALID) == 0) {
            *entry = 0;
        }
        ++entry;
    } while (--remaining != 0);
}
