#include <stdint.h>

enum {
    FAULT_COUNT = 64,
    HISTORY_AGED_THIS_WARMUP = 0x80,
};

static __sfr __at (0x92) DPSEL;

static volatile __xdata __at (0xf512) uint8_t fault_history[FAULT_COUNT];
static volatile __xdata __at (0xf552) uint8_t fault_history_timers[FAULT_COUNT];
static volatile __xdata __at (0xf9b7) uint8_t coolant_warmup_flags;

void age_fault_history_timers(void)
{
    const uint8_t flags = coolant_warmup_flags;
    if (flags == 0 || (flags & HISTORY_AGED_THIS_WARMUP)) {
        return;
    }

    for (uint8_t i = 0; i < FAULT_COUNT; ++i) {
        uint8_t left = fault_history_timers[i];
        if (left != 0) {
            fault_history_timers[i] = --left;
            if (left == 0) {
                fault_history[i] = 0;
            }
        }
    }
    coolant_warmup_flags |= HISTORY_AGED_THIS_WARMUP;
}
