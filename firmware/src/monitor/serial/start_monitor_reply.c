#include "stock_registers.h"

static __sfr __at (0x98) SCON;
static __sbit __at (0x08) monitor_frame_pending;
static __sbit __at (0x09) monitor_transmitting;

extern void finish_monitor_reply(void);

void start_monitor_reply(void)
{
    finish_monitor_reply();
    if (monitor_frame_pending) {
        monitor_transmitting = 1;
        monitor_frame_pending = 0;
        SCON |= 2;
    }
}
