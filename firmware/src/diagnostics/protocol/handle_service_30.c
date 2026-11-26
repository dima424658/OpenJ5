#include "stock_registers.h"

static __sbit __at (0x56) fuel_mode_flag_56;
static __sbit __at (0x6b) service_link_ready_6b;
static __sbit __at (0x7d) service_session_active;
static __sbit __at (0x7e) service_request_seen;
static __sbit __at (0x7f) serial_extended_length;
static volatile __data __at (0x04) uint8_t stock_r4;
static volatile __xdata __at (0xf680) uint8_t serial_rx_buffer[128];

extern __code const uint8_t output_unavailable_reply[];
extern __code const uint8_t output_invalid_reply[];

extern void reset_serial_transmit_buffer(void);
extern void append_code_string_to_serial(void);
extern void set_flag_2f_4_cd7c(void);

extern void apply_output_control_01_to_04(void);
extern void apply_output_control_05(void);
extern void apply_output_control_06(void);
extern void apply_output_control_09(void);
extern void apply_output_control_0a(void);
extern void apply_output_control_0b(void);
extern void apply_output_control_0c(void);
extern void apply_output_control_0d(void);
extern void apply_output_control_41(void);
extern void apply_output_control_42(void);
extern void build_output_control_reply(void);
extern void build_extended_control_reply(void);

#define APPEND_CONTROL_REPLY(table)                          \
    do {                                                     \
        const uint16_t address = (uint16_t)(table);          \
        stock_r3 = address >> 8;                             \
        stock_r4 = (uint8_t)address;                         \
        append_code_string_to_serial();                      \
    } while (0)

void handle_service_30(void)
{
    reset_serial_transmit_buffer();
    if (!service_link_ready_6b || !service_session_active) {
        goto unavailable;
    }

    service_request_seen = 1;
    const uint8_t offset = serial_extended_length ? 1 : 0;
    stock_r3 = serial_rx_buffer[5 + offset];
    stock_r4 = serial_rx_buffer[4 + offset];
    if (stock_r4 == 0) {
        goto unavailable;
    }
    if (stock_r4 < 5) {
        apply_output_control_01_to_04();
        goto reply;
    }

    switch (stock_r4) {
    case 0x05:
        if (fuel_mode_flag_56) {
            goto unavailable;
        }
        apply_output_control_05();
        goto reply;
    case 0x06:
        if (fuel_mode_flag_56) {
            goto unavailable;
        }
        apply_output_control_06();
        goto reply;
    case 0x09:
        if (fuel_mode_flag_56) {
            goto unavailable;
        }
        apply_output_control_09();
        goto reply;
    case 0x0a:
        apply_output_control_0a();
        goto reply;
    case 0x0b:
        apply_output_control_0b();
        goto reply;
    case 0x0c:
        apply_output_control_0c();
        goto reply;
    case 0x0d:
        apply_output_control_0d();
        goto reply;
    case 0x41:
        apply_output_control_41();
        break;
    case 0x42:
        apply_output_control_42();
        break;
    default:
        APPEND_CONTROL_REPLY(output_invalid_reply);
        set_flag_2f_4_cd7c();
        return;
    }
    build_extended_control_reply();
    set_flag_2f_4_cd7c();
    return;

reply:
    build_output_control_reply();
    set_flag_2f_4_cd7c();
    return;

unavailable:
    APPEND_CONTROL_REPLY(output_unavailable_reply);
    set_flag_2f_4_cd7c();
}
