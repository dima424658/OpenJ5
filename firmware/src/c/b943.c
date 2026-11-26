#include "stock_registers.h"

static volatile __data __at (0x20) uint8_t state_mask;
static volatile __data __at (0x21) uint8_t state_history[4];

#define MIX_HISTORY_BYTE()                            \
    do {                                              \
        stock_r0 = stock_r2;                          \
        ACC = state_history[3];                       \
        state_history[3] = state_history[2];          \
        state_history[2] = state_history[1];          \
        state_history[1] = state_history[0];          \
        ACC = (uint8_t)((ACC >> 1) | (ACC << 7));     \
        ACC ^= state_mask;                            \
        ACC ^= *(volatile __idata uint8_t *)stock_r0; \
        state_history[0] = ACC;                       \
    } while (0)

void mix_next_extra_state_byte(void)
{
    MIX_HISTORY_BYTE();
    if (stock_r0 == 0x70) {
        stock_r2 = 0x54;
    } else {
        ++stock_r2;
    }
}

void mix_extra_state_byte(void)
{
    MIX_HISTORY_BYTE();
}
