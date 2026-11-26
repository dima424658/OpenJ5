#include "stock_registers.h"

static __sbit __at (0x7c) service_dispatch_7c;
static __sbit __at (0x7d) service_session_active;
static __sbit __at (0x7e) service_request_seen;
static __sbit __at (0x7f) serial_extended_length;

static volatile __data __at (0x04) uint8_t stock_r4;
static volatile __data __at (0x79) uint8_t output_override_flags;
static volatile __data __at (0x7a) uint8_t ignition_override;
static volatile __data __at (0x7b) uint8_t knock_override;
static volatile __data __at (0x7c) uint8_t output_flags;
static volatile __data __at (0x7d) uint8_t sensor_override;
static volatile __idata __at (0x91) uint8_t serial_rx_state;
static volatile __idata __at (0xa5) uint8_t serial_default_reload;
static volatile __xdata __at (0xf680) uint8_t serial_rx_buffer[128];

extern __code const uint8_t link_start_reply[];
extern __code const uint8_t link_stop_reply[];
extern __code const uint8_t session_start_reply[];
extern __code const uint8_t session_stop_reply[];
extern __code const uint8_t service_11_reply[];
extern __code const uint8_t link_stop_inactive_reply[];
extern __code const uint8_t session_start_denied_reply[];
extern __code const uint8_t session_stop_inactive_reply[];
extern __code const uint8_t service_11_inactive_reply[];
extern __code const uint8_t session_mode_invalid_reply[];

extern void restart_serial_receive(void);
extern void reset_serial_transmit_buffer(void);
extern void append_code_string_to_serial(void);

#define APPEND_SESSION_REPLY(table)                          \
    do {                                                     \
        const uint16_t address = (uint16_t)(table);          \
        stock_r3 = address >> 8;                             \
        stock_r4 = (uint8_t)address;                         \
        append_code_string_to_serial();                      \
    } while (0)

#define CLEAR_OUTPUT_OVERRIDES()                             \
    do {                                                     \
        output_override_flags = 0;                           \
        ignition_override = 0;                               \
        knock_override = 0;                                  \
        output_flags = 0;                                    \
        sensor_override = 0;                                 \
    } while (0)

void start_diagnostic_link(void)
{
    if (service_session_active) {
        restart_serial_receive();
    } else {
        service_session_active = 1;
        reset_serial_transmit_buffer();
        APPEND_SESSION_REPLY(link_start_reply);
        service_dispatch_7c = 1;
    }
}

void stop_diagnostic_link(void)
{
    reset_serial_transmit_buffer();
    if (service_session_active) {
        CLEAR_OUTPUT_OVERRIDES();
        APPEND_SESSION_REPLY(link_stop_reply);
        service_session_active = 0;
    } else {
        APPEND_SESSION_REPLY(link_stop_inactive_reply);
    }
    service_dispatch_7c = 1;
}

void start_diagnostic_session(void)
{
    reset_serial_transmit_buffer();
    if (!service_session_active || service_request_seen) {
        APPEND_SESSION_REPLY(session_start_denied_reply);
    } else {
        const uint8_t offset = serial_extended_length ? 1 : 0;
        stock_r2 = serial_rx_buffer[5 + offset];
        if (serial_rx_buffer[4 + offset] != 0x81) {
            APPEND_SESSION_REPLY(session_mode_invalid_reply);
        } else {
            service_request_seen = 1;
            APPEND_SESSION_REPLY(session_start_reply);
            if (stock_r2 == 0x26) {
                serial_default_reload = 0xf3;
            } else if (stock_r2 == 0x39) {
                serial_default_reload = 0xf7;
            }
        }
    }
    service_dispatch_7c = 1;
}

void stop_diagnostic_session(void)
{
    reset_serial_transmit_buffer();
    if (service_session_active) {
        service_request_seen = 0;
        CLEAR_OUTPUT_OVERRIDES();
        APPEND_SESSION_REPLY(session_stop_reply);
        serial_default_reload = 0xd0;
    } else {
        APPEND_SESSION_REPLY(session_stop_inactive_reply);
    }
    service_dispatch_7c = 1;
}

void handle_diagnostic_service_11(void)
{
    reset_serial_transmit_buffer();
    if (service_session_active) {
        APPEND_SESSION_REPLY(service_11_reply);
        serial_rx_state |= 0x20;
    } else {
        APPEND_SESSION_REPLY(service_11_inactive_reply);
    }
    service_dispatch_7c = 1;
}
