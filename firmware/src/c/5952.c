#include "stock_registers.h"

static volatile __data __at (1) uint16_t stock_r12;

/* R2:R1 is the unsigned accumulator of a filter with a 1/16 update weight.
 * ACC supplies the latest sample and R3 supplies the previous rounded output.
 */
void filter_maf_baseline(void)
{
    const uint16_t sum = stock_r12 + ACC - stock_r3;
    stock_r12 = sum;

    const uint16_t rounded = sum + 8;
    stock_r3 = (uint8_t)((rounded >> 8) << 4);
    ACC = (uint8_t)(rounded >> 4);
}
