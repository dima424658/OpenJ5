#ifndef JANVAR_RETAINED_RAM_H
#define JANVAR_RETAINED_RAM_H

#include "stock_registers.h"

static volatile __xdata __at (0xf4d2) uint8_t retained_entries[64];
static volatile __xdata __at (0xfd00) uint8_t retained_table_pages[512];
static volatile __xdata __at (0xf8fe) uint16_t retained_ram_checksum;

/* Expand at each stock entry point without emitting an out-of-line copy. */
#define ACCUMULATE_RETAINED_RAM()                                                \
    do {                                                                        \
        const volatile __xdata uint8_t *entry = retained_table_pages;            \
        stock_r01 = 1;                                                          \
        do {                                                                    \
            stock_r01 += *entry++;                                              \
        } while (entry != retained_table_pages + sizeof(retained_table_pages)); \
                                                                                \
        entry = retained_entries;                                               \
        uint8_t remaining = sizeof(retained_entries);                            \
        do {                                                                    \
            stock_r01 += *entry++;                                              \
        } while (--remaining != 0);                                             \
    } while (0)

#endif
