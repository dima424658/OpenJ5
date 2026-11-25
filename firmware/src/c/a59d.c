#include "stock_registers.h"

enum {
    AXIS_STEP = 16,
    FRACTION_MASK = AXIS_STEP - 1,
    ROW_MASK = 0xf0,
    ROW_STRIDE = 16,
};

static volatile __data __at (4) uint8_t stock_r4;
static __sfr16 __at (0x8382) STOCK_DPTR;

/* DPTR addresses a row-major 16 by 16 table in CODE. R2 and R3 supply
 * the column and row in Q4.4. A receives the high byte of the weighted sum.
 */
void interp_bilinear_u8(void)
{
    /* R4 holds the top-left offset; R2 and R3 retain the fractional parts. */
    stock_r4 = (stock_r3 & ROW_MASK) + (stock_r2 >> 4);
    stock_r2 &= FRACTION_MASK;
    stock_r3 &= FRACTION_MASK;

    const __code uint8_t *table = (const __code uint8_t *)STOCK_DPTR;
    if ((stock_r2 | stock_r3) == 0) {
        ACC = table[stock_r4];
        return;
    }

    /* The weights sum to 256, with each weight fitting in one byte on this
     * path. The original table offsets wrap at one byte.
     */
    const uint8_t bottom_right = stock_r2 * stock_r3;
    stock_r01 = (uint16_t)table[(uint8_t)(stock_r4 + ROW_STRIDE + 1)] * bottom_right;

    const uint8_t bottom_left = (AXIS_STEP - stock_r2) * stock_r3;
    stock_r01 += (uint16_t)table[(uint8_t)(stock_r4 + ROW_STRIDE)] * bottom_left;

    stock_r3 = AXIS_STEP - stock_r3;
    const uint8_t top_right = stock_r2 * stock_r3;
    stock_r01 += (uint16_t)table[(uint8_t)(stock_r4 + 1)] * top_right;

    const uint8_t top_left = (AXIS_STEP - stock_r2) * stock_r3;
    const uint16_t total = stock_r01 + (uint16_t)table[stock_r4] * top_left;
    stock_r0 = (uint8_t)total;
    ACC = total >> 8;
}
