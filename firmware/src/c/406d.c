#include <stdint.h>

static volatile __data __at (0x5f) uint8_t rpm_charge_index;
static volatile __xdata __at (0xf87d) uint8_t engine_torque;

extern __code const uint8_t engine_torque_raw[256];
extern void load_ignition_tail_continue(void);

void update_engine_torque(void)
{
    engine_torque = engine_torque_raw[rpm_charge_index];

    load_ignition_tail_continue();
}
