#include <stdint.h>

static volatile __data __at (0x58) uint8_t rpm_nearest_half_bin;
static volatile __xdata __at (0xf943) uint8_t knock_threshold;
static volatile __xdata __at (0xf953) uint8_t knock_min_interval;
static volatile __xdata __at (0xf954) uint8_t advance_recovery_period;

extern __code const uint8_t knock_threshold_by_rpm[32];
extern __code const uint8_t knock_min_cycle_interval[32];
extern __code const uint8_t knock_advance_recovery_period[32];
extern void update_fuel_weighting(void);

void update_knock_rpm_parameters(void)
{
    knock_threshold = knock_threshold_by_rpm[rpm_nearest_half_bin];
    knock_min_interval = knock_min_cycle_interval[rpm_nearest_half_bin];
    advance_recovery_period = knock_advance_recovery_period[rpm_nearest_half_bin];

    update_fuel_weighting();
}
