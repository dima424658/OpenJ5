#include <stdint.h>

enum { STARTER_RELAY_BIT = 1 << 5 };

static __sfr __at (0xe0) ACC;
static __sfr __at (0xf0) B;
static __sbit __at (0x49) tick_1s;
static __sbit __at (0x58) crank_stopped;
static volatile __data __at (0x4e) uint8_t coolant_nearest_index;
static volatile __data __at (0x53) uint8_t rpm_half;
static volatile __xdata __at (0xf95b) uint8_t starter_elapsed_seconds;
static volatile __xdata __at (0xf95c) uint8_t starter_above_rpm_cycles;
static volatile __xdata __at (0xf999) uint8_t relay_outputs;

extern __code const uint8_t starter_max_seconds[39];
extern __code const uint8_t starter_cutoff_rpm_half[39];
extern __code const uint8_t starter_cutoff_delay_cycles[39];
extern void update_knock_noise_zone(void);

void rpm_update_continued(void)
{
    if (crank_stopped) {
        update_knock_noise_zone();
        return;
    }

    ACC = starter_elapsed_seconds;
    if (tick_1s) {
        ++ACC;
        if (ACC != 0)
            starter_elapsed_seconds = ACC;
    }
    /* The latched counter retains 255 on wrap, while the comparison uses 0. */
    B = ACC;
    if (starter_max_seconds[coolant_nearest_index] < B) {
        relay_outputs &= (uint8_t)~STARTER_RELAY_BIT;
        update_knock_noise_zone();
        return;
    }

    B = rpm_half;
    if (starter_cutoff_rpm_half[coolant_nearest_index] >= B) {
        starter_above_rpm_cycles = 0;
        update_knock_noise_zone();
        return;
    }

    ACC = starter_above_rpm_cycles + 1;
    if (ACC != 0)
        starter_above_rpm_cycles = ACC;

    B = ACC;
    if (starter_cutoff_delay_cycles[coolant_nearest_index] < B)
        relay_outputs &= (uint8_t)~STARTER_RELAY_BIT;
    update_knock_noise_zone();
}
