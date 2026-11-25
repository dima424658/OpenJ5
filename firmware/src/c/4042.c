#include <stdint.h>

static volatile __data __at (0x4e) uint8_t coolant_nearest_index;
static volatile __data __at (0x5f) uint8_t operating_point_index;
static volatile __xdata __at (0xf876) uint8_t load_warmup_correction_raw;
static volatile __xdata __at (0xf878) uint8_t warmup_duration;
static volatile __xdata __at (0xf807) uint8_t warmup_elapsed;

extern __code const int8_t load_warmup_ignition[39];
extern __code const uint8_t warmup_ignition_weight_raw[256];
extern void update_engine_torque(void);

void update_load_warmup_ignition(void)
{
    const uint8_t duration = warmup_duration;
    uint8_t result;
    if (warmup_elapsed >= duration) {
        result = 0;
    } else {
        /* The stock multiplier interprets both raw bytes as unsigned. */
        const uint8_t correction = load_warmup_ignition[coolant_nearest_index];
        const uint8_t weight = warmup_ignition_weight_raw[operating_point_index];
        result = ((uint16_t)correction * weight + 128u) >> 8;
    }
    load_warmup_correction_raw = result;

    update_engine_torque();
}
