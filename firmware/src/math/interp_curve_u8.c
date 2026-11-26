#include "stock_registers.h"

static __sfr16 __at (0x8382) STOCK_DPTR;

/* The stock ABI passes the curve in DPTR, its index in R2 and the fractional
 * weight in R3. The result is in A. The right index wraps at one byte.
 */
void interp_curve_u8(void)
{
    const __code uint8_t *curve = (const __code uint8_t *)STOCK_DPTR;
    stock_r0 = curve[stock_r2];
    const uint8_t right = curve[(uint8_t)(stock_r2 + 1)];
    const uint8_t left = stock_r0;
    const int16_t delta = (int16_t)right - left;

    if (delta >= 0) {
        /* Round the fraction of the increase to the nearest byte step. */
        const uint16_t scaled = (uint8_t)delta * (uint16_t)stock_r3;
        ACC = left + ((scaled + 128u) >> 8);
    } else {
        /* The stock decreasing branch truncates the magnitude. */
        const uint16_t scaled = (uint8_t)-delta * (uint16_t)stock_r3;
        ACC = left - (scaled >> 8);
    }
}
