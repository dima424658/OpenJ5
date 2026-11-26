#include <stdint.h>

static volatile __data __at (0x4e) uint8_t coolant_nearest_index;
static volatile __data __at (0x58) uint8_t rpm_nearest_half_bin;
static volatile __data __at (0x63) uint8_t idle_ignition_target_raw;
static volatile __xdata __at (0xf878) uint8_t warmup_duration;
static volatile __xdata __at (0xf807) uint8_t warmup_elapsed;

extern __code const int8_t idle_ignition_by_rpm[32];
extern __code const int8_t idle_ignition_temp_correction[39];
extern __code const int8_t idle_warmup_ignition[39];
extern void update_idle_airflow(void);

/* The angle is stored as a byte in half-degrees, with wrapping arithmetic. */
void update_idle_ignition(void)
{
    const uint8_t temp_correction = idle_ignition_temp_correction[coolant_nearest_index];
    const uint8_t base = idle_ignition_by_rpm[rpm_nearest_half_bin];
    const uint8_t corrected = base - temp_correction;

    const uint8_t duration = warmup_duration;
    uint8_t warmup_correction;
    if (warmup_elapsed >= duration) {
        warmup_correction = 0;
    } else {
        warmup_correction = idle_warmup_ignition[coolant_nearest_index];
    }
    idle_ignition_target_raw = corrected + warmup_correction;

    update_idle_airflow();
}
