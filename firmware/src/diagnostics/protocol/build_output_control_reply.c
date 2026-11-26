#include "output_control_reply.h"

extern __code const uint8_t output_range_reply[];

void build_output_control_reply(void)
{
    if (stock_r2 == 0x12) {
        APPEND_OUTPUT_ERROR_REPLY(output_invalid_reply);
    } else if (stock_r2 == 0x31) {
        APPEND_OUTPUT_ERROR_REPLY(output_range_reply);
    } else {
        const uint8_t saved_dpsel = DPSEL;
        WRITE_OUTPUT_REPLY_HEADER(0x84);
        serial_tx_buffer[6] = stock_r2;
        serial_tx_count = 7;
        DPSEL = saved_dpsel;
        append_serial_checksum();
    }
}
