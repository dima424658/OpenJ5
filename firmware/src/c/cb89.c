#include "stock_registers.h"

static __sfr __at (0xf0) B;
static __sbit __at (0x7c) service_dispatch_7c;
static __sbit __at (0x7d) service_session_active;
static __sbit __at (0x7e) service_request_seen;
static __sbit __at (0x7f) serial_extended_length;

static volatile __data __at (0x04) uint8_t stock_r4;
static volatile __xdata __at (0xf680) uint8_t serial_rx_buffer[128];

extern __code const uint8_t faults_cleared_group_00[];
extern __code const uint8_t faults_cleared_group_ff[];
extern __code const uint8_t fault_clear_invalid_group[];
extern __code const uint8_t fault_clear_inactive[];
extern __code const uint8_t fault_read_invalid_group[];
extern __code const uint8_t fault_read_inactive[];

extern void reset_serial_transmit_buffer(void);
extern void append_code_string_to_serial(void);
extern void append_code_bytes_to_serial(void);
extern void clear_diagnostic_faults(void);
extern void build_diagnostic_fault_response(void);

#define SELECT_RESPONSE(table)                               \
    do {                                                     \
        const uint16_t address = (uint16_t)(table);          \
        stock_r3 = address >> 8;                             \
        stock_r4 = (uint8_t)address;                         \
    } while (0)

void handle_service_14(void)
{
    reset_serial_transmit_buffer();
    if (!service_session_active) {
        SELECT_RESPONSE(fault_clear_inactive);
        append_code_string_to_serial();
    } else {
        service_request_seen = 1;
        const uint8_t group = serial_rx_buffer[4 + (serial_extended_length ? 1 : 0)];
        if (group == 0) {
            clear_diagnostic_faults();
            SELECT_RESPONSE(faults_cleared_group_00);
            stock_r2 = 7;
            append_code_bytes_to_serial();
        } else if (group == 0xff) {
            clear_diagnostic_faults();
            SELECT_RESPONSE(faults_cleared_group_ff);
            stock_r2 = 7;
            append_code_bytes_to_serial();
        } else {
            SELECT_RESPONSE(fault_clear_invalid_group);
            append_code_string_to_serial();
        }
    }
    service_dispatch_7c = 1;
}

void handle_service_18(void)
{
    reset_serial_transmit_buffer();
    if (!service_session_active) {
        SELECT_RESPONSE(fault_read_inactive);
        append_code_string_to_serial();
    } else {
        service_request_seen = 1;
        const uint8_t offset = serial_extended_length ? 1 : 0;
        B = serial_rx_buffer[5 + offset];
        if (serial_rx_buffer[4 + offset] == 0 && (B == 0 || B == 0xff)) {
            build_diagnostic_fault_response();
        } else {
            SELECT_RESPONSE(fault_read_invalid_group);
            append_code_string_to_serial();
        }
    }
    service_dispatch_7c = 1;
}
