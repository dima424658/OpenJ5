#include "stock_registers.h"

static __sfr __at (0xf0) B;
static __sfr __at (0x82) STOCK_DPL;
static __sfr16 __at (0x8382) STOCK_DPTR;

/* The second byte is in the same XDATA page even when DPL starts at 0xFF. */
void accumulate_u8_xdata_u16(void)
{
    B = ACC;
    volatile __xdata uint8_t *low = (volatile __xdata uint8_t *)STOCK_DPTR;
    const uint16_t sum = (uint16_t)*low + B;
    *low = (uint8_t)sum;

    ++STOCK_DPL;
    volatile __xdata uint8_t *high = (volatile __xdata uint8_t *)STOCK_DPTR;
    ACC = *high + (uint8_t)(sum >> 8);
    *high = ACC;
}

void interp_desc_curve_minus_20(void)
{
    stock_r1 = ACC;
    const __code uint8_t *curve = (const __code uint8_t *)STOCK_DPTR;
    B = ACC >> 4;
    stock_r0 = curve[B];
    B = curve[B + 1];
    B = stock_r0 - B;

    ACC = (stock_r1 & 0x0f) << 4;
    B = (uint16_t)ACC * B >> 8;

    const uint8_t interpolated = stock_r0 - B;
    ACC = interpolated >= 20 ? interpolated - 20 : 0;
}
