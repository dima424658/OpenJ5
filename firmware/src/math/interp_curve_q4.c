#include "stock_registers.h"

static __sfr16 __at (0x8382) STOCK_DPTR;

/* The stock ABI takes a Q4.4 coordinate in R2 and a CODE curve in DPTR.
 * R3 receives the left index on the 32-point grid and R2 the fraction in 256ths.
 * The interpolated byte is returned in A.
 */
void interp_curve_q4(void)
{
    const uint8_t position = stock_r2;
    stock_r3 = position >> 3;
    stock_r2 = (position & 7) << 5;

    const __code uint8_t *curve = (const __code uint8_t *)STOCK_DPTR;
    if (stock_r2 == 0) {
        ACC = curve[stock_r3];
        return;
    }

    stock_r01 = (uint16_t)curve[(uint8_t)(stock_r3 + 1)] * stock_r2;
    const uint16_t left = (uint16_t)curve[stock_r3] * (uint8_t)-stock_r2;
    const uint16_t weighted = stock_r01 + left;
    stock_r0 = (uint8_t)weighted;
    ACC = weighted >> 8;
}
