#include "stock_registers.h"

enum {
    KNOCK_PHASE_PERIOD = 30,
    KNOCK_PHASE_ORIGIN = 11,
    KNOCK_SECOND_PAIR_OFFSET = 15,
};

static __sbit __at (0xaf) interrupts_enabled;

static volatile __data __at (0x4a) uint8_t knock_begin_pair_2;
static volatile __data __at (0x4c) uint8_t knock_begin_pair_1;
static volatile __data __at (0x56) uint8_t knock_end_pair_2;
static volatile __data __at (0x74) uint8_t knock_end_pair_1;
static volatile __xdata __at (0xf946) uint8_t knock_window_start;
static volatile __xdata __at (0xf947) uint8_t knock_window_end;

extern void continue_cycle_fuel(void);

/* Convert the clamped window phase to the interrupt's cyclic origin. */
#define LOAD_WINDOW_PHASE(phase)                            \
    do {                                                    \
        ACC = phase;                                        \
        if (ACC >= KNOCK_PHASE_PERIOD) {                    \
            ACC = KNOCK_PHASE_PERIOD - 1;                   \
        }                                                   \
        phase = ACC;                                        \
        if (ACC >= KNOCK_PHASE_ORIGIN) {                    \
            ACC -= KNOCK_PHASE_ORIGIN;                      \
        } else {                                            \
            ACC += KNOCK_PHASE_PERIOD - KNOCK_PHASE_ORIGIN; \
        }                                                   \
    } while (0)

#define OFFSET_SECOND_PAIR()             \
    do {                                 \
        ACC += KNOCK_SECOND_PAIR_OFFSET; \
        if (ACC >= KNOCK_PHASE_PERIOD) { \
            ACC -= KNOCK_PHASE_PERIOD;   \
        }                                \
    } while (0)

void apply_knock_window_phases(void)
{
    LOAD_WINDOW_PHASE(knock_window_start);
    stock_r0 = ACC;
    OFFSET_SECOND_PAIR();
    stock_r1 = ACC;

    LOAD_WINDOW_PHASE(knock_window_end);
    stock_r2 = ACC;
    OFFSET_SECOND_PAIR();

    interrupts_enabled = 0;
    knock_end_pair_2 = ACC;
    knock_end_pair_1 = stock_r2;
    knock_begin_pair_2 = stock_r1;
    knock_begin_pair_1 = stock_r0;
    interrupts_enabled = 1;
    continue_cycle_fuel();
}
