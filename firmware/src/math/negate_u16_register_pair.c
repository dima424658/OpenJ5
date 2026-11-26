#include "stock_registers.h"

static volatile __data __at (0) unsigned _BitInt(24) stock_r012;

/* Arithmetic wraps at the operand width, with CY holding the borrow. */
void negate_u16_register_pair(void)
{
    stock_r01 = -stock_r01;
}

void negate_u24_register_triple(void)
{
    stock_r012 = -stock_r012;
}

/* Extend the signed value in R2:R1:R0 into R3 without changing CY. */
void sign_extend_i24_register_triple(void)
{
    ACC = stock_r2;
    if (ACC_SIGN) {
        stock_r3 = UINT8_MAX;
    } else {
        stock_r3 = 0;
    }
}
