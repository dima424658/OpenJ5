#include "stock_registers.h"

static __sfr __at (0xf0) B;
static volatile __data __at (4) uint8_t stock_r4;
static volatile __xdata __at (0xf4bb) uint8_t received_pulse_count;
static volatile __xdata __at (0xf4bc) uint8_t received_nibble_count;
static volatile __xdata __at (0xf4c1) uint8_t received_record[3];
static volatile __xdata __at (0xf4c5) uint8_t immobilizer_status;

#define MERGE_HIGH_NIBBLE(byte)                       \
    do {                                              \
        const uint8_t previous = (byte);              \
        ACC = B;                                      \
        B = previous;                                 \
        ACC = (uint8_t)((ACC << 4) | (ACC >> 4)) | B; \
        (byte) = ACC;                                 \
    } while (0)

void store_immobilizer_nibble(void)
{
    B = received_pulse_count;
    stock_r4 = received_nibble_count;

    ACC = stock_r4 ^ 1;
    if (ACC == 0) {
        received_record[0] = B;
        return;
    }
    ACC = stock_r4 ^ 2;
    if (ACC == 0) {
        MERGE_HIGH_NIBBLE(received_record[0]);
        return;
    }
    ACC = stock_r4 ^ 3;
    if (ACC == 0) {
        received_record[1] = B;
        return;
    }
    ACC = stock_r4 ^ 4;
    if (ACC == 0) {
        MERGE_HIGH_NIBBLE(received_record[1]);
        return;
    }
    ACC = stock_r4 ^ 5;
    if (ACC == 0) {
        received_record[2] = B;
        return;
    }
    ACC = stock_r4 ^ 6;
    if (ACC == 0) {
        MERGE_HIGH_NIBBLE(received_record[2]);
        immobilizer_status &= 0xf7;
    }
}
