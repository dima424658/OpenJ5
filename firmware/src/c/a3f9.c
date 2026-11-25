#include "stock_registers.h"

/* R1:R0 is updated in place. CY holds the carry or borrow. */
void add_u16_register_pairs(void)
{
    stock_r01 += stock_r23;
}

void subtract_u16_register_pairs(void)
{
    stock_r01 -= stock_r23;
}

/* Extend R0 into R1 without changing CY. */
void sign_extend_i8_register(void)
{
    ACC = stock_r0;
    if (!ACC_SIGN) {
        stock_r1 = 0;
    } else {
        stock_r1 = UINT8_MAX;
    }
}
