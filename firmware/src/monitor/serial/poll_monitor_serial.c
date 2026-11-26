#include "stock_registers.h"

static __sfr __at (0x92) DPSEL;
static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0x08) monitor_frame_pending;
static __sbit __at (0xaf) EA;

static volatile __xdata __at (0xfa00) uint8_t monitor_rx_buffer[128];
static volatile __xdata __at (0xfa80) uint8_t monitor_reply_buffer[256];

extern void service_monitor_uart_byte(void);
extern void clear_flag_21_0_f275(void);
extern void dispatch_monitor_frame(void);

void poll_monitor_serial(void)
{
    service_monitor_uart_byte();
    EA = 0;
    if (!monitor_frame_pending) {
        EA = 1;
        return;
    }

    DPSEL = 6;
    const uint8_t length = (uint8_t)STOCK_DPTR;
    STOCK_DPTR = (uint16_t)monitor_rx_buffer;
    EA = 1;
    DPSEL = 0;
    if (length < 2) {
        clear_flag_21_0_f275();
        return;
    }

    stock_r0 = length;
    stock_r3 = length - 2;
    stock_r2 = 0;
    STOCK_DPTR = (uint16_t)monitor_rx_buffer - 1;
    do {
        ++STOCK_DPTR;
        ACC = *(volatile __xdata uint8_t *)STOCK_DPTR;
        stock_r2 += ACC;
    } while (--stock_r0);
    ACC = stock_r2;
    if (ACC != 0) {
        clear_flag_21_0_f275();
        return;
    }

    *(volatile __xdata uint8_t *)STOCK_DPTR = 0;
    DPSEL = 7;
    STOCK_DPTR = (uint16_t)monitor_reply_buffer;
    DPSEL = 0;
    stock_r2 = 0;
    STOCK_DPTR = (uint16_t)monitor_rx_buffer;
    ACC = monitor_rx_buffer[0];
    if (ACC == 0) {
        clear_flag_21_0_f275();
        return;
    }
    dispatch_monitor_frame();
}
