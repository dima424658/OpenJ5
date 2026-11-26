#include "stock_registers.h"

static __sbit __at (0x56) diagnostic_write_inhibited;
static __sbit __at (0x7c) service_dispatch_7c;
static __sbit __at (0x7d) service_session_active;
static __sbit __at (0x7e) service_request_seen;
static __sbit __at (0x7f) serial_extended_length;

static volatile __data __at (0x04) uint8_t stock_r4;
static volatile __xdata __at (0xf680) uint8_t serial_rx_buffer[128];

extern __code const uint8_t service_3b_inactive_reply[];
extern __code const uint8_t service_3b_invalid_reply[];

extern void reset_serial_transmit_buffer(void);
extern void append_code_string_to_serial(void);
extern void store_identification_90(void);
extern void store_identification_98(void);
extern void store_diag_record_a1(void);
extern void store_diag_record_a2(void);
extern void store_diag_record_a3(void);

#define APPEND_REPLY(reply)                                 \
    do {                                                    \
        const uint16_t address = (uint16_t)(reply);          \
        stock_r3 = address >> 8;                            \
        stock_r4 = (uint8_t)address;                        \
        append_code_string_to_serial();                     \
    } while (0)

void handle_service_3b(void)
{
    reset_serial_transmit_buffer();
    if (service_session_active) {
        service_request_seen = 1;
        if (!diagnostic_write_inhibited) {
            const uint8_t offset = serial_extended_length ? 1 : 0;
            const uint8_t record = serial_rx_buffer[4 + offset];
            if (record == 0x90) {
                store_identification_90();
            } else if (record == 0x98) {
                store_identification_98();
            } else if (record == 0xa1) {
                store_diag_record_a1();
            } else if (record == 0xa2) {
                store_diag_record_a2();
            } else if (record == 0xa3) {
                store_diag_record_a3();
            } else {
                APPEND_REPLY(service_3b_invalid_reply);
            }
            service_dispatch_7c = 1;
            return;
        }
    }
    APPEND_REPLY(service_3b_inactive_reply);
    service_dispatch_7c = 1;
}
