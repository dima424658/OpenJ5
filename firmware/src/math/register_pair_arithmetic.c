#include "stock_registers.h"

#ifdef MONITOR_ADD_COPY
#define ADD_REGISTER_PAIRS add_monitor_register_pairs
#else
#define ADD_REGISTER_PAIRS add_u16_register_pairs
#endif

/* R1:R0 is updated in place. CY holds the carry or borrow. */
void ADD_REGISTER_PAIRS(void)
{
    stock_r01 += stock_r23;
}

#ifndef MONITOR_ADD_COPY
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
#endif
