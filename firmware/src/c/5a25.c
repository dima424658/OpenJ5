#include <stdint.h>

static __sfr __at (0x92) DPSEL;
static __sfr __at (0x99) SBUF;
static __sfr __at (0xbe) serial_control_be;
static __sfr16 __at (0x8382) STOCK_DPTR;

static __sbit __at (0x78) serial_transmit_done;
static __sbit __at (0x79) service_request_79;
static __sbit __at (0x7a) serial_length_latched;
static __sbit __at (0x7b) service_event_pending;
static __sbit __at (0x7f) serial_extended_length;
static __sbit __at (0x98) serial_receive_pending;
static __sbit __at (0x99) serial_transmit_pending;
static __sbit __at (0x9c) serial_receive_enabled;

static volatile __data __at (0x51) uint8_t serial_tx_count;
static volatile __data __at (0x73) uint8_t serial_expected_length;
static volatile __data __at (0x78) uint8_t service_event_timer;
static volatile __data __at (0x7e) uint8_t serial_rx_count;

static volatile __xdata __at (0xf680) uint8_t serial_rx_buffer[128];

void service_uart0_transmit(void);
void uart0_restore_context(void);
void uart0_reti(void);

void uart0_interrupt(void) __naked
{
    if (!serial_receive_pending) {
        service_uart0_transmit();
        return;
    }
    serial_receive_pending = 0;
    __asm
        push psw
        push acc
        push _DPSEL
    __endasm;

    service_event_timer = 0;
    if (!serial_rx_count) {
        service_request_79 = 0;
        serial_control_be |= 8;
    }
    if (serial_rx_count != 128) {
        DPSEL = 6;
        *(volatile __xdata uint8_t *)STOCK_DPTR = SBUF;
        ++STOCK_DPTR;
        ++serial_rx_count;

        if (!serial_length_latched) {
            if (serial_rx_count >= 4) {
                serial_length_latched = 1;
                const uint16_t cursor = STOCK_DPTR;
                const uint8_t length = serial_rx_buffer[0] & 0x3f;
                if (length) {
                    STOCK_DPTR = cursor;
                    serial_expected_length = length + 4;
                    serial_extended_length = 0;
                } else {
                    const uint8_t extended_length = serial_rx_buffer[3];
                    STOCK_DPTR = cursor;
                    serial_expected_length = extended_length + 5;
                    serial_extended_length = 1;
                }
            }
            goto restore_context;
        }
        if (serial_rx_count < serial_expected_length) {
            goto restore_context;
        }
    }

    serial_receive_enabled = 0;
    serial_control_be &= 0xf7;
    serial_length_latched = 0;
    service_event_timer = 0;
    service_event_pending = 1;

restore_context:
    uart0_restore_context();
}

void service_uart0_transmit(void) __naked
{
    if (!serial_transmit_pending) {
        goto no_context;
    }
    serial_transmit_pending = 0;
    if (serial_transmit_done) {
        goto no_context;
    }
    __asm
        push psw
        push acc
        push _DPSEL
    __endasm;

    if (serial_tx_count) {
        DPSEL = 7;
        SBUF = *(const volatile __xdata uint8_t *)STOCK_DPTR;
        ++STOCK_DPTR;
        --serial_tx_count;
    } else {
        serial_transmit_done = 1;
        service_event_timer = 0;
        service_request_79 = 1;
    }
    uart0_restore_context();
    return;

no_context:
    uart0_reti();
}

void uart0_restore_context(void) __naked
{
    __asm
        pop _DPSEL
        pop acc
        pop psw
    __endasm;
    uart0_reti();
}

void uart0_reti(void) __naked
{
    __asm reti __endasm;
}
