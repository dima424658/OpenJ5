#include "stock_registers.h"

static __sbit __at (0x7c) service_dispatch_7c;
static __sbit __at (0x7d) service_session_active;
static __sbit __at (0x7e) service_request_seen;
static volatile __data __at (0x04) uint8_t stock_r4;

extern __code const uint8_t identification_inactive_reply[];
extern void reset_serial_transmit_buffer(void);
extern void append_code_string_to_serial(void);
extern void build_identification_response(void);

void handle_service_1a(void)
{
    reset_serial_transmit_buffer();
    if (service_session_active) {
        service_request_seen = 1;
        build_identification_response();
    } else {
        const uint16_t address = (uint16_t)identification_inactive_reply;
        stock_r3 = address >> 8;
        stock_r4 = (uint8_t)address;
        append_code_string_to_serial();
    }
    service_dispatch_7c = 1;
}
