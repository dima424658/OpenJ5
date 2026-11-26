#include "stock_registers.h"

enum {
    RECEIVE_TIMEOUT = 0x3c,
    RECEIVE_START_LEVEL = 0x50,
    RECEIVE_END_LEVEL = 0x0a,
    NIBBLE_GAP_TICKS = 0xfa,
};

static __sbit __at (0xe7) ACC_BIT7;
static __sbit __at (0x49) immobilizer_input;
static __sbit __at (0x70) immobilizer_signal;
static volatile __data __at (0x52) uint8_t throttle_position;
static volatile __xdata __at (0xf4bb) uint8_t received_pulse_count;
static volatile __xdata __at (0xf4bc) uint8_t received_nibble_count;
static volatile __xdata __at (0xf4c5) uint8_t immobilizer_status;
static volatile __xdata __at (0xf4c6) uint8_t message_timeout;
static volatile __xdata __at (0xf4c7) uint8_t pulse_elapsed_ticks;
static volatile __xdata __at (0xf4c8) uint8_t pulse_phase;

extern void store_immobilizer_nibble(void);

void poll_immobilizer_pulses(void)
{
    ACC = immobilizer_status;
    if (!ACC_BIT7) {
        return;
    }
    if (immobilizer_input) {
        ACC = message_timeout + 1;
        message_timeout = ACC;
        if (ACC == RECEIVE_TIMEOUT) {
            immobilizer_status &= 0x7f;
            return;
        }
    }

    if (pulse_phase == 0 && throttle_position > RECEIVE_START_LEVEL) {
        immobilizer_signal = 0;
        ACC = pulse_elapsed_ticks + 1;
        pulse_elapsed_ticks = ACC;
        if (ACC >= NIBBLE_GAP_TICKS) {
            immobilizer_signal = 1;
            pulse_phase = 1;
            pulse_elapsed_ticks = 0;
            goto accept_nibble;
        }

        message_timeout = 0;
        return;
    }

    if (throttle_position > RECEIVE_END_LEVEL) {
        return;
    }

    immobilizer_signal = 1;
    pulse_phase = 0;
    if (pulse_elapsed_ticks == 0) {
        return;
    }

    received_pulse_count = (received_pulse_count + 1) & 0x0f;
    ACC = 0;
    pulse_elapsed_ticks = ACC;
    message_timeout = ACC;
    return;

accept_nibble:
    ACC = received_pulse_count;
    if (ACC == 0) {
        immobilizer_status &= 0xeb;
    } else if (ACC >= 0x0a) {
        immobilizer_status &= 0xfb;
    }
    ++received_nibble_count;
    store_immobilizer_nibble();
    ACC = 0;
    pulse_elapsed_ticks = ACC;
    received_pulse_count = ACC;
    message_timeout = ACC;
}
