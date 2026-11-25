#include "stock_registers.h"

/* R0 and R2 supply the samples, R3 supplies the weight. R1:R0 is scratch.
 * The complementary weight wraps at one byte, so zero yields zero.
 */
void blend_u8_q8(void)
{
    const uint8_t weight = stock_r3;
    stock_r01 = (uint8_t)-weight * (uint16_t)stock_r0;
    const uint16_t second_product = weight * (uint16_t)stock_r2;
    ACC = (stock_r01 + second_product) >> 8;
}
