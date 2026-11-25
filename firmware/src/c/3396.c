#include <stdint.h>

enum {
    COOLANT_AXIS_STEP = 5,
    COOLANT_AXIS_LAST_BASE_INDEX = 36,
    COOLANT_FRACTION_SCALE = 51,
};

/* The raw temperature code is degrees Celsius plus 40. */
static volatile __data __at (0x49) uint8_t coolant_temperature_raw;
static volatile __data __at (0x4e) uint8_t coolant_nearest_index;
static volatile __xdata __at (0xf887) uint8_t idle_temp_index;
static volatile __xdata __at (0xf888) uint8_t idle_temp_fraction;

extern void coolant_update_continued(void);

/* Entered within the sensor update. The continuation shares its stack frame. */
void update_coolant_axis(void)
{
    uint8_t nearest = (uint8_t)(coolant_temperature_raw + COOLANT_AXIS_STEP / 2)
        / COOLANT_AXIS_STEP;
    if (nearest >= COOLANT_AXIS_LAST_BASE_INDEX) {
        nearest = COOLANT_AXIS_LAST_BASE_INDEX;
    }
    coolant_nearest_index = nearest;

    const uint8_t coolant = coolant_temperature_raw;
    uint8_t index = coolant / COOLANT_AXIS_STEP;
    if (index >= COOLANT_AXIS_LAST_BASE_INDEX) {
        index = COOLANT_AXIS_LAST_BASE_INDEX;
    }
    idle_temp_index = index;
    idle_temp_fraction = (coolant % COOLANT_AXIS_STEP) * COOLANT_FRACTION_SCALE;

    coolant_update_continued();
}
