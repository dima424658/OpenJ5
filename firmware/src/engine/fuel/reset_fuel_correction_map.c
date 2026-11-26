#include "stock_registers.h"

static __sfr __at (0x92) DPSEL;
static __sfr16 __at (0x8382) STOCK_DPTR;

extern __code const uint8_t initial_injection_correction[256];
static volatile __xdata __at (0xfd00) uint8_t injection_correction_map[256];

void reset_fuel_correction_map(void)
{
    /* The caller enters with pointer bank 0 selected. */
    STOCK_DPTR = (uint16_t)initial_injection_correction;
    DPSEL = 2;
    STOCK_DPTR = (uint16_t)injection_correction_map;
    DPSEL = 0;

    /* Zero in the decrementing byte counter represents all 256 entries. */
    stock_r0 = 0;
    do {
        ACC = *(const __code uint8_t *)STOCK_DPTR;
        ++STOCK_DPTR;
        DPSEL = 2;
        *(volatile __xdata uint8_t *)STOCK_DPTR = ACC;
        ++STOCK_DPTR;
        DPSEL = 0;
    } while (--stock_r0 != 0);
}
