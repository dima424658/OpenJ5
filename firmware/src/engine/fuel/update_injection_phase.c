#include <stdint.h>

enum { HOLD_INJECTION_PHASE = 0x02 };

static volatile __data __at (0x7a) uint8_t injection_control_7a;
static volatile __data __at (0x5f) uint8_t rpm_charge_index;
static volatile __data __at (0x4d) uint8_t injection_phase;

extern __code const uint8_t injection_phase_raw[256];
extern void update_knock_threshold_scale(void);

void update_injection_phase(void)
{
    if (!(injection_control_7a & HOLD_INJECTION_PHASE)) {
        injection_phase = injection_phase_raw[rpm_charge_index];
    }

    update_knock_threshold_scale();
}
