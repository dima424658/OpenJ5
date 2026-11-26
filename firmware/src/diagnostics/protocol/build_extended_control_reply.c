#include "output_control_reply.h"

static volatile __data __at (0x53) uint8_t rpm_half;
static volatile __data __at (0x55) uint8_t rpm_eighth;
static volatile __data __at (0x64) uint8_t iac_target;
static volatile __data __at (0x65) uint8_t iac_position;
static volatile __xdata __at (0xf849) uint8_t idle_rpm_target;

void build_extended_control_reply(void)
{
    if (stock_r2 == 0x12) {
        APPEND_OUTPUT_ERROR_REPLY(output_invalid_reply);
    } else {
        const uint8_t saved_dpsel = DPSEL;
        WRITE_OUTPUT_REPLY_HEADER(0x87);
        serial_tx_buffer[6] = iac_target;
        serial_tx_buffer[7] = iac_position;
        serial_tx_buffer[8] = rpm_half == UINT8_MAX ? rpm_eighth : rpm_half;
        DPSEL = saved_dpsel;
        const uint8_t idle_rpm = idle_rpm_target;
        DPSEL = 7;
        serial_tx_buffer[9] = idle_rpm;
        DPSEL = saved_dpsel;
        serial_tx_count = 10;
        append_serial_checksum();
    }
}

void build_control_reply_r1(void)
{
    if (stock_r2 == 0x12) {
        APPEND_OUTPUT_ERROR_REPLY(output_invalid_reply);
    } else {
        const uint8_t saved_dpsel = DPSEL;
        WRITE_OUTPUT_REPLY_HEADER(0x84);
        serial_tx_buffer[6] = stock_r1;
        serial_tx_count = 7;
        DPSEL = saved_dpsel;
        append_serial_checksum();
    }
}
