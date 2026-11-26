#include "stock_registers.h"

static __sfr __at (0x92) DPSEL;
static __sfr16 __at (0x8382) STOCK_DPTR;

static volatile __xdata __at (0xf512) uint8_t retained_words_low[64];
static volatile __xdata __at (0xf552) uint8_t retained_words_high[64];

/* A already holds the fill byte. The two pointer banks advance in parallel. */
void clear_retained_words_ce94(void)
{
    stock_r2 = sizeof(retained_words_low);
    DPSEL = 2;
    STOCK_DPTR = (uint16_t)retained_words_high;
    DPSEL = 0;
    STOCK_DPTR = (uint16_t)retained_words_low;
    do {
        *(volatile __xdata uint8_t *)STOCK_DPTR = ACC;
        ++STOCK_DPTR;
        DPSEL = 2;
        *(volatile __xdata uint8_t *)STOCK_DPTR = ACC;
        ++STOCK_DPTR;
        DPSEL = 0;
    } while (--stock_r2 != 0);
}
