#include "stock_registers.h"

static __sbit __at (0x7c) service_dispatch_7c;
static __sbit __at (0x7d) service_session_active;
static __sbit __at (0x7e) service_request_seen;
static __sbit __at (0x7f) serial_extended_length;

static volatile __data __at (0x04) uint8_t stock_r4;
static volatile __data __at (0x78) uint8_t service_event_timer;
static volatile __idata __at (0x92) uint8_t serial_tx_state;
static volatile __xdata __at (0xf680) uint8_t serial_rx_buffer[128];

extern __code const uint8_t tester_present_reply[];
extern __code const uint8_t service_3e_inactive_reply[];

extern void reset_serial_transmit_buffer(void);
extern void restart_serial_receive(void);
extern void append_code_string_to_serial(void);
extern void append_code_bytes_to_serial(void);

#define SELECT_REPLY(template)                               \
    do {                                                     \
        const uint16_t address = (uint16_t)(template);       \
        stock_r3 = address >> 8;                             \
        stock_r4 = (uint8_t)address;                         \
    } while (0)

void handle_service_3e(void)
{
    if (service_session_active) {
        service_request_seen = 1;
        const uint8_t offset = serial_extended_length ? 1 : 0;
        if (serial_rx_buffer[4 + offset] != 1) {
            serial_tx_state = 0;
            service_event_timer = 0;
            restart_serial_receive();
            return;
        }
        reset_serial_transmit_buffer();
        SELECT_REPLY(tester_present_reply);
        /* Include the checksum even though its value is zero. */
        stock_r2 = 5;
        append_code_bytes_to_serial();
    } else {
        reset_serial_transmit_buffer();
        SELECT_REPLY(service_3e_inactive_reply);
        append_code_string_to_serial();
    }
    service_dispatch_7c = 1;
}
