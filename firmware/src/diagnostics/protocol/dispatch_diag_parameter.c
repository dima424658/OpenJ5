#include "stock_registers.h"

static __sbit __at (0x7c) service_dispatch_7c;
static __sbit __at (0x7d) service_session_active;
static __sbit __at (0x7e) service_request_seen;
static __sbit __at (0x7f) serial_extended_length;

static volatile __data __at (0x04) uint8_t stock_r4;
static volatile __xdata __at (0xf680) uint8_t serial_rx_buffer[128];

extern __code const uint8_t diag_inactive_response[];
extern __code const uint8_t diag_unknown_parameter[];

extern void reset_serial_transmit_buffer(void);
extern void append_code_string_to_serial(void);
extern void build_full_diag_response(void);
extern void build_short_diag_response(void);
extern void build_adc_diag_response(void);
extern void build_misfire_diag_response(void);
extern void build_diag_record_a0(void);
extern void build_diag_record_a1(void);
extern void build_diag_record_a2(void);
extern void build_diag_record_a3(void);
extern void build_diag_record_e0(void);

#define APPEND_DIAG_REPLY(template)                          \
    do {                                                     \
        const uint16_t address = (uint16_t)(template);       \
        stock_r3 = address >> 8;                             \
        stock_r4 = (uint8_t)address;                         \
        append_code_string_to_serial();                      \
    } while (0)

void dispatch_diag_parameter(void)
{
    reset_serial_transmit_buffer();
    if (!service_session_active) {
        APPEND_DIAG_REPLY(diag_inactive_response);
    } else {
        service_request_seen = 1;
        stock_r4 = serial_extended_length ? serial_rx_buffer[5] : serial_rx_buffer[4];
        switch (stock_r4) {
        case 0x01: build_full_diag_response(); break;
        case 0x02: build_short_diag_response(); break;
        case 0x03: build_adc_diag_response(); break;
        case 0x04: build_misfire_diag_response(); break;
        case 0xa0: build_diag_record_a0(); break;
        case 0xa1: build_diag_record_a1(); break;
        case 0xa2: build_diag_record_a2(); break;
        case 0xa3: build_diag_record_a3(); break;
        case 0xe0: build_diag_record_e0(); break;
        default: APPEND_DIAG_REPLY(diag_unknown_parameter); break;
        }
    }
    service_dispatch_7c = 1;
}
