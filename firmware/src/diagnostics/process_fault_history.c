#include <stdint.h>

enum {
    FAULT_COUNT = 64,
    FAULTS_PER_MASK = 8,
    CONFIRMED = 0x80,
    OBSERVED = 0xff,
    CANDIDATE_TIMEOUT = 20,
};

static __sfr __at (0x92) DPSEL;
static __sbit __at (0x0b) history_sync_inhibit_0b;
static __sbit __at (0x49) tick_1s;
static __sbit __at (0x4a) tick_1min;

static volatile __xdata __at (0xf4ca) uint8_t pending_masks[FAULT_COUNT / FAULTS_PER_MASK];
static volatile __xdata __at (0xf4d2) uint8_t fault_state[FAULT_COUNT];
static volatile __xdata __at (0xf512) uint8_t fault_history[FAULT_COUNT];
static volatile __xdata __at (0xf552) uint8_t fault_history_timers[FAULT_COUNT];

extern volatile __code const uint8_t fault_history_initial_values[FAULT_COUNT];
extern volatile __code const uint8_t fault_clear_after_decrement;
extern volatile __code const uint8_t active_fault_history_lifetime;

void refresh_active_fault_history(void);
extern void update_lamp_and_outputs(void);

void process_fault_history(void)
{
    if (tick_1s) {
        uint8_t mask = 0;
        for (uint8_t i = 0; i < FAULT_COUNT; ++i) {
            if (i % FAULTS_PER_MASK == 0) {
                mask = pending_masks[i / FAULTS_PER_MASK];
                pending_masks[i / FAULTS_PER_MASK] = 0;
            }

            const uint8_t observed = mask & 1;
            mask >>= 1;
            const uint8_t state = fault_state[i];
            if (observed) {
                if (state == 0) {
                    fault_state[i] = 1;
                    continue;
                }
                fault_state[i] = OBSERVED;
            } else if (!(state & CONFIRMED)) {
                if (state != 0) {
                    const uint8_t next = state + 1;
                    fault_state[i] = next < CANDIDATE_TIMEOUT ? next : 0;
                }
                continue;
            }
            fault_history[i] = fault_history_initial_values[i];
        }
    }

    refresh_active_fault_history();
}

void refresh_active_fault_history(void)
{
    if (!history_sync_inhibit_0b && tick_1s) {
        const uint8_t lifetime = active_fault_history_lifetime;
        const uint8_t clear_after_decrement = fault_clear_after_decrement;
        for (uint8_t i = 0; i < FAULT_COUNT; ++i) {
            uint8_t state = fault_state[i];
            if (state & CONFIRMED) {
                fault_history_timers[i] = lifetime;
                if (tick_1min) {
                    --state;
                    if (state == clear_after_decrement) {
                        fault_history[i] &= ~CONFIRMED;
                        state = 0;
                    }
                }
            }
            fault_state[i] = state;
        }
    }

    update_lamp_and_outputs();
}
