#include "stock_registers.h"

static __sfr __at (0xf0) B;
static __sfr16 __at (0x8382) STOCK_DPTR;

/* A provides the Q4.4 coordinate and DPTR points to the leftmost CODE sample. */
void interp_descending_curve_q4(void)
{
    stock_r1 = ACC;
    const __code uint8_t *curve = (const __code uint8_t *)STOCK_DPTR;
    B = ACC >> 4;
    stock_r0 = curve[B];
    B = curve[B + 1];
    B = stock_r0 - B;

    ACC = (stock_r1 & 0x0f) << 4;
    B = (uint16_t)ACC * B >> 8;
    const uint8_t left = stock_r0;
    ACC = left >= B ? left - B : 0;
}
