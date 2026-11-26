#include "stock_registers.h"

static volatile __data __at (4) uint8_t stock_r4;
static volatile __xdata __at (0xf999) uint8_t relay_output_latch;
static volatile __xdata __at (0xf9b2) uint8_t injector_low_or_open_faults;
static volatile __xdata __at (0xf9b3) uint8_t injector_high_and_evap_faults;
static volatile __xdata __at (0xf9b4) uint8_t driver_diagnostics_f9b4;
static volatile __xdata __at (0xf9b8) uint8_t injector_driver_response;
static volatile __xdata __at (0xf9b9) uint8_t evap_driver_response;
static volatile __xdata __at (0xf9ba) uint8_t relay_driver_response;
static volatile __xdata __at (0xf9bc) uint8_t relay_bit6_mismatch_count;
static volatile __xdata __at (0xf9bd) uint8_t relay_bit1_mismatch_count;
static volatile __xdata __at (0xfa6b) uint8_t misfire_enabled_flags;

extern volatile __code const uint8_t relay_bit1_fault_delay;
extern volatile __code const uint8_t relay_bit6_fault_delay;

/* Codes 0 through 3 mean ground short, supply short, open and clear. */
#define DECODE_INJECTOR(mask, open_code, channel_bit)                        \
    do {                                                                     \
        stock_r3 = stock_r4 & (mask);                                        \
        if (stock_r3 != (mask)) {                                            \
            if (stock_r3 == 0) {                                             \
                injector_low_or_open_faults |= (channel_bit) << 4;           \
            } else if (stock_r3 == (open_code)) {                            \
                injector_low_or_open_faults |= (channel_bit);                \
            } else {                                                         \
                injector_high_and_evap_faults |= (channel_bit);              \
            }                                                                \
        }                                                                    \
    } while (0)

void decode_driver_diagnostics(void)
{
    injector_low_or_open_faults = 0;
    injector_high_and_evap_faults = 0;
    driver_diagnostics_f9b4 &= 0xf0;

    stock_r4 = injector_driver_response;
    if (stock_r4 == UINT8_MAX) {
        misfire_enabled_flags = UINT8_MAX;
    } else {
        misfire_enabled_flags = 0;
        DECODE_INJECTOR(0xc0, 0x80, 1);
        DECODE_INJECTOR(0x30, 0x20, 2);
        DECODE_INJECTOR(0x03, 0x02, 4);
        DECODE_INJECTOR(0x0c, 0x08, 8);
    }

    stock_r4 = evap_driver_response & 0x30;
    if (stock_r4 != 0x30) {
        injector_high_and_evap_faults |= 0x10;
        if (stock_r4 == 0) {
            injector_high_and_evap_faults |= 0x40;
        } else if (stock_r4 == 0x20) {
            injector_high_and_evap_faults |= 0x20;
        } else {
            injector_high_and_evap_faults |= 0x80;
        }
    }

    stock_r3 = relay_output_latch & 0x42;
    stock_r4 = relay_driver_response & 0x42;
    const uint8_t disagreement = stock_r3 ^ stock_r4;
    if (disagreement == 0) {
        relay_bit6_mismatch_count = 0;
        relay_bit1_mismatch_count = 0;
        return;
    }
    /* Evaluate a channel only when its own bit agrees and the other differs. */
    if (!(disagreement & 0x02)) {
        ACC = relay_bit1_mismatch_count + 1;
        relay_bit1_mismatch_count = ACC;
        if (ACC >= relay_bit1_fault_delay) {
            driver_diagnostics_f9b4 |= 0x01;
        }
    }
    if (!(disagreement & 0x40)) {
        ACC = relay_bit6_mismatch_count + 1;
        relay_bit6_mismatch_count = ACC;
        if (ACC >= relay_bit6_fault_delay) {
            driver_diagnostics_f9b4 |= (stock_r4 & 0x40) ? 0x08 : 0x02;
        }
    }
}
