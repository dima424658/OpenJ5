#include "stock_registers.h"

static __sfr __at (0x92) DPSEL;
static __sfr16 __at (0x8382) STOCK_DPTR;
static __sfr __at (0xf0) B;
static __sbit __at (0x7c) service_dispatch;
static __sbit __at (0x7d) service_session_active;
static __sbit __at (0x7e) service_request_seen;
static __sbit __at (0x7f) serial_extended_length;
static volatile __data __at (4) uint8_t stock_r4;
static volatile __data __at (0x51) uint8_t serial_tx_count;
static volatile __xdata __at (0xf680) uint8_t serial_rx_buffer[128];
static volatile __xdata __at (0xf600) uint8_t serial_tx_buffer[128];

extern void reset_serial_transmit_buffer(void);
extern void append_code_string_to_serial(void);
extern void copy_xdata_bytes(void);
extern void append_serial_checksum(void);

#define READ_REQUEST(offset)                       \
    do {                                           \
        B = serial_rx_buffer[(offset)];            \
        stock_r1 = serial_rx_buffer[(offset) + 1]; \
        stock_r3 = serial_rx_buffer[(offset) + 2]; \
        stock_r2 = serial_rx_buffer[(offset) + 3]; \
    } while (0)

void handle_service_23(void)
{
    reset_serial_transmit_buffer();
    if (!service_session_active) {
        goto inactive;
    }
    service_request_seen = 1;

    if (serial_extended_length) {
        READ_REQUEST(5);
    } else {
        READ_REQUEST(4);
    }

    ACC = B;
    if (ACC != 0) {
        goto invalid_selector;
    }

    ACC = stock_r2;
    if (ACC == 0 || ACC >= 0x78) {
        goto invalid_range;
    }
    if (stock_r1 >= 0xfd) {
        if (stock_r1 == 0xff) {
            goto invalid_range;
        }
    } else {
        ACC = 0xf9 ^ stock_r1;
        if (ACC != 0 || stock_r3 >= 0x30) {
            goto invalid_range;
        }
    }

    ACC = 0x80;
    const uint8_t saved_dpsel = DPSEL;
    DPSEL = 7;
    serial_tx_buffer[0] = ACC;
    stock_r0 = 0xa6;
    ACC = *(volatile __idata uint8_t *)stock_r0;
    serial_tx_buffer[1] = ACC;
    serial_tx_buffer[2] = 0x10;
    serial_tx_buffer[3] = stock_r2 + 1;
    serial_tx_buffer[4] = 0x63;
    serial_tx_count = stock_r2 + 5;
    DPSEL = 4;
    STOCK_DPTR = (uint16_t)&serial_tx_buffer[5];
    DPSEL = 3;
    STOCK_DPTR = ((uint16_t)stock_r1 << 8) | stock_r3;
    copy_xdata_bytes();
    DPSEL = saved_dpsel;
    append_serial_checksum();
    goto done;

invalid_range:
    stock_r3 = 0xc8;
    stock_r4 = 0xaa;
    append_code_string_to_serial();
    goto done;

invalid_selector:
    stock_r3 = 0xc8;
    stock_r4 = 0x92;
    append_code_string_to_serial();
    goto done;

inactive:
    stock_r3 = 0xc8;
    stock_r4 = 0x4a;
    append_code_string_to_serial();

done:
    service_dispatch = 1;
}
