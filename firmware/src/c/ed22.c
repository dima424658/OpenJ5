#include "monitor_port9_io.h"
#include "stock_registers.h"

static __sfr __at (0x82) STOCK_DPL;
static __sfr __at (0x83) STOCK_DPH;
static __sfr __at (0xdd) sfr_dd;
static __sfr __at (0xf0) B;
static __sfr __at (0xf9) P9;
static __sbit __at (0x00) monitor_command_active;
static __sbit __at (0x01) monitor_flag_01;
static __sbit __at (0xae) iac_tick_interrupt_enabled;
static __sbit __at (0xbe) iac_tick_high_priority;
static __sbit __at (0xd7) CY;

static volatile __data __at (0x7c) uint8_t monitor_latched_events[2];
static volatile __data __at (0x7e) uint8_t monitor_transfer_bytes[2];

extern void resume_service_monitor(void);

/* On exit, the instruction sequence discards the caller's return address
 * and enters the monitor without adding another stack frame.
 */
#define POLL_MONITOR_EXIT()                             \
    do {                                                \
        ACC = sfr_dd & 8;                               \
        if (ACC != 0) {                                 \
            monitor_command_active = 0;                 \
            monitor_flag_01 = 1;                        \
            resume_service_monitor();                   \
            return;                                     \
        }                                               \
        iac_tick_interrupt_enabled = 1;                 \
        iac_tick_high_priority = 1;                     \
    } while (0)

void poll_monitor_exit(void)
{
    POLL_MONITOR_EXIT();
}

void poll_monitor_exit_and_io(void)
{
    POLL_MONITOR_EXIT();

    const uint8_t saved_b = B;
    const uint8_t saved_r0 = stock_r0;
    const uint8_t saved_r1 = stock_r1;

    P9 &= 0xfb;
    ACC = monitor_transfer_bytes[0];
    STOCK_DPL = monitor_transfer_bytes[1];
    monitor_transfer_bytes[1] = ACC;
    SHIFT_PORT9_BYTE();

    /* Record a bit only when it matches the previous sample. */
    STOCK_DPH = (uint8_t)~(ACC ^ STOCK_DPL);
    monitor_latched_events[0] |= (uint8_t)~STOCK_DPL & STOCK_DPH;
    monitor_latched_events[1] |= STOCK_DPL & STOCK_DPH;
    P9 |= 4;

    stock_r1 = saved_r1;
    ACC = saved_r0;
    stock_r0 = ACC;
    B = saved_b;
}
