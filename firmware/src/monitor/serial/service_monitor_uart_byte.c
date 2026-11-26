#include "stock_registers.h"

static __sfr __at (0x92) DPSEL;
static __sfr __at (0x98) SCON;
static __sfr __at (0x99) SBUF;
static __sfr __at (0xd0) PSW;
static __sfr16 __at (0x8382) STOCK_DPTR;

static __sbit __at (0x08) monitor_frame_pending;
static __sbit __at (0x09) monitor_transmitting;
static __sbit __at (0x0a) monitor_escape_pending;
static __sbit __at (0x0b) monitor_flag_0b;
static __sbit __at (0x0c) monitor_flag_0c;

static volatile __data __at (0x70) uint8_t monitor_reply_end_low;
static volatile __xdata __at (0xfa00) uint8_t monitor_rx_buffer[128];

enum {
    UART_RECEIVE_READY = 1,
    UART_TRANSMIT_READY = 2,
    UART_RECEIVE_MARK = 4,
    MONITOR_ESCAPE = 0x40,
    MONITOR_END_OF_FRAME = 0x0d,
};

void service_monitor_uart_byte(void)
{
    const uint8_t saved_psw = PSW;
    const uint8_t saved_acc = ACC;
    const uint8_t saved_dpsel = DPSEL;
    uint8_t serial_flags = SCON;

    if (serial_flags & UART_TRANSMIT_READY) {
        SCON &= (uint8_t)~UART_TRANSMIT_READY;
        DPSEL = 7;
        if (monitor_escape_pending) {
            monitor_escape_pending = 0;
            ACC = *(volatile __xdata uint8_t *)STOCK_DPTR - MONITOR_ESCAPE;
            ++STOCK_DPTR;
            SBUF = ACC;
        } else {
            const uint8_t end_low = monitor_reply_end_low;
            const uint16_t end = (end_low < 0x82 ? 0xfb00 : 0xfa00) | end_low;
            if (STOCK_DPTR == end) {
                ++STOCK_DPTR;
                SBUF = MONITOR_END_OF_FRAME;
            } else if (STOCK_DPTR < end) {
                ACC = *(volatile __xdata uint8_t *)STOCK_DPTR;
                if (ACC == MONITOR_ESCAPE || ACC == MONITOR_END_OF_FRAME) {
                    monitor_escape_pending = 1;
                    SBUF = MONITOR_ESCAPE;
                } else {
                    ++STOCK_DPTR;
                    SBUF = ACC;
                }
            } else {
                monitor_transmitting = 0;
            }
        }

        serial_flags = SCON;
        if (!(serial_flags & UART_RECEIVE_READY)) {
            goto restore_registers;
        }
        SCON &= (uint8_t)~UART_RECEIVE_READY;
        if (serial_flags & UART_RECEIVE_MARK) {
            goto restore_registers;
        }
    } else {
        if (!(serial_flags & UART_RECEIVE_READY)) {
            goto restore_registers;
        }
        SCON &= (uint8_t)~UART_RECEIVE_READY;
        if (serial_flags & UART_RECEIVE_MARK) {
            if (monitor_frame_pending || monitor_transmitting) {
                goto restore_registers;
            }
            DPSEL = 6;
            if ((uint8_t)STOCK_DPTR == 0x80) {
                STOCK_DPTR = (uint16_t)monitor_rx_buffer;
                goto restore_registers;
            }

            ACC = SBUF;
            if (monitor_escape_pending) {
                monitor_escape_pending = 0;
                if (ACC == 0) {
                    ACC = MONITOR_ESCAPE;
                } else if (ACC == 0xcd) {
                    ACC = MONITOR_END_OF_FRAME;
                } else {
                    goto reset_receiver;
                }
            } else if (ACC == MONITOR_ESCAPE) {
                monitor_escape_pending = 1;
                goto restore_registers;
            } else if (ACC == MONITOR_END_OF_FRAME) {
                if ((uint8_t)STOCK_DPTR != 0) {
                    monitor_frame_pending = 1;
                }
                goto restore_registers;
            }

            *(volatile __xdata uint8_t *)STOCK_DPTR = ACC;
            ++STOCK_DPTR;
            goto restore_registers;
        }
    }

reset_receiver:
    monitor_flag_0b = 0;
    monitor_flag_0c = 0;
    monitor_frame_pending = 0;
    monitor_transmitting = 0;
    DPSEL = 6;
    STOCK_DPTR = (uint16_t)monitor_rx_buffer;
    monitor_escape_pending = 0;

restore_registers:
    DPSEL = saved_dpsel;
    ACC = saved_acc;
    PSW = saved_psw;
}

void unused_monitor_uart_irq(void) __interrupt
{
}
