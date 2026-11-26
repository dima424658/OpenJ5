#include "stock_registers.h"

static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0xd7) CY;

/* A zero count uses the stock DJNZ convention and consumes 256 words. */
#define SUM_MONITOR_WORDS()                                                   \
    do {                                                                      \
        stock_r01 = 1;                                                        \
        do {                                                                  \
            const uint8_t low = *(volatile __xdata uint8_t *)STOCK_DPTR;      \
            ++STOCK_DPTR;                                                     \
            const uint8_t high = *(volatile __xdata uint8_t *)STOCK_DPTR;     \
            ++STOCK_DPTR;                                                     \
            stock_r01 += ((uint16_t)high << 8) | low;                         \
        } while (--stock_r2);                                                 \
    } while (0)

void verify_monitor_word_checksum(void)
{
    SUM_MONITOR_WORDS();
    ACC = stock_r1 | stock_r0;
    CY = ACC != 0;
}

void append_monitor_word_checksum(void)
{
    SUM_MONITOR_WORDS();
    const uint16_t checksum = -stock_r01;
    *(volatile __xdata uint8_t *)STOCK_DPTR = (uint8_t)checksum;
    ++STOCK_DPTR;
    ACC = checksum >> 8;
    *(volatile __xdata uint8_t *)STOCK_DPTR = ACC;
}
