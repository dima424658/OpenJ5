#include "stock_registers.h"

static volatile __xdata __at (0xf4bb) uint8_t received_pulse_count;
static volatile __xdata __at (0xf4bc) uint8_t received_nibble_count;
static volatile __xdata __at (0xf4bd) uint8_t selected_record[4];
static volatile __xdata __at (0xf4c1) uint8_t second_record[4];
static volatile __xdata __at (0xf4c5) uint8_t immobilizer_status;
static volatile __xdata __at (0xf4c6) uint8_t message_timeout;
static volatile __xdata __at (0xf4c7) uint8_t pulse_elapsed_ticks;
static volatile __xdata __at (0xf4c8) uint8_t pulse_phase;

void initialize_immobilizer_records(void)
{
    ACC = 0;
    received_pulse_count = ACC;
    received_nibble_count = ACC;
    pulse_elapsed_ticks = ACC;
    pulse_phase = ACC;

    ACC = UINT8_MAX;
    selected_record[0] = ACC;
    selected_record[1] = ACC;
    selected_record[2] = ACC;
    second_record[0] = ACC;
    second_record[1] = ACC;
    second_record[2] = ACC;
    immobilizer_status = ACC;

    ACC = 0x3c;
    message_timeout = ACC;
}
