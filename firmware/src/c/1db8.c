#include "injector_timer.h"

static __sfr __at (0xf0) B;
static __sbit __at (0x50) split_injection_dose;

static volatile __data __at (0x0b) uint8_t bank_one_r3;
static volatile __data __at (0x41) uint8_t crank_slot;
static volatile __data __at (0x7c) uint8_t output_flags_7c;

extern __code const uint8_t injector_channels_by_slot[4];
extern void continue_after_injection(void);

#define SCHEDULE_INJECTOR(channel, output, compare_low, compare_high)                 \
    do {                                                                              \
        const uint8_t mask = 1u << (channel);                                         \
        if (!(output_flags_7c & mask) &&                                              \
            ((channel) == bank_one_r3 || (split_injection_dose && (channel) == B))) { \
            START_INJECTOR_PULSE(mask, channel, output, compare_low, compare_high);   \
        }                                                                             \
    } while (0)

void schedule_injector_pulses(void)
{
    stock_r3 = crank_slot;
    if (split_injection_dose) {
        B = injector_channels_by_slot[stock_r3 ^ 2];
    }
    stock_r3 = injector_channels_by_slot[stock_r3];

    // In register bank 1 the primary channel is the physical byte at IRAM:0B.
    SCHEDULE_INJECTOR(0, injector_output_0, CML0, CMH0);
    SCHEDULE_INJECTOR(1, injector_output_1, CML1, CMH1);
    SCHEDULE_INJECTOR(2, injector_output_2, CML2, CMH2);
    SCHEDULE_INJECTOR(3, injector_output_3, CML3, CMH3);
    continue_after_injection();
}
