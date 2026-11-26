#include "stock_registers.h"

enum {
    AXIS_STEP = 16,
    FRACTION_MASK = AXIS_STEP - 1,
    ROW_MASK = 0xf0,
    ROW_STRIDE = 16,
};

static volatile __data __at (4) uint8_t stock_r4;

#ifdef BILINEAR_XDATA
static __sfr __at (0x83) STOCK_DPH;
#define INTERP_FUNCTION interp_bilinear_xdata
#define TABLE_MEMORY __xdata
#define TABLE_ADDRESS ((uint16_t)STOCK_DPH << 8)
#else
static __sfr16 __at (0x8382) STOCK_DPTR;
#define INTERP_FUNCTION interp_bilinear_u8
#define TABLE_MEMORY __code
#define TABLE_ADDRESS STOCK_DPTR
#endif

/* R2 and R3 supply the column and row of a row-major 16 by 16 table in Q4.4.
 * The CODE variant keeps the full base in DPTR. The XDATA variant uses DPH
 * as a page number and writes the requested byte offsets to DPL.
 * A receives the high byte of the weighted sum.
 */
void INTERP_FUNCTION(void)
{
    /* R4 holds the top-left offset; R2 and R3 retain the fractional parts. */
    stock_r4 = (stock_r3 & ROW_MASK) + (stock_r2 >> 4);
    stock_r2 &= FRACTION_MASK;
    stock_r3 &= FRACTION_MASK;

    const TABLE_MEMORY uint8_t *table = (const TABLE_MEMORY uint8_t *)TABLE_ADDRESS;
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
