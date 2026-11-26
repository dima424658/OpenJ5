#include "stock_registers.h"

/* R1:R0 holds the result, R3:R2 is the other operand.
 * CY reports whether the other operand was below the original value.
 */
void minimum_u16_register_pair(void)
{
    if (stock_r23 < stock_r01) {
        stock_r01 = stock_r23;
    }
}

void maximum_u16_register_pair(void)
{
    if (stock_r23 >= stock_r01) {
        stock_r01 = stock_r23;
    }
}
