#include "stock_registers.h"

static __sfr __at (0x92) DPSEL;
static __sfr16 __at (0x8382) STOCK_DPTR;

static volatile __data __at (0x70) uint8_t monitor_reply_end_low;
static volatile __xdata __at (0xfa80) uint8_t monitor_reply_buffer[256];

extern void clear_flag_21_0_f275(void);
extern void select_data_pointer_zero(void);

void append_monitor_reply_byte(void)
{
    const uint8_t saved_dpsel = DPSEL;
    DPSEL = 7;
    *(volatile __xdata uint8_t *)STOCK_DPTR = ACC;
    ++STOCK_DPTR;
    stock_r2 += ACC;
    if (STOCK_DPTR == 0xfb81) {
        DPSEL = saved_dpsel;
        /* The stock path discards the caller's immediate return address. */
        clear_flag_21_0_f275();
        return;
    }
    DPSEL = saved_dpsel;
}

void finish_monitor_reply(void)
{
    ACC = (uint8_t)-stock_r2;
    DPSEL = 7;
    *(volatile __xdata uint8_t *)STOCK_DPTR = ACC;
    ++STOCK_DPTR;
    monitor_reply_end_low = (uint8_t)STOCK_DPTR;
    STOCK_DPTR = (uint16_t)monitor_reply_buffer;
    select_data_pointer_zero();
}
