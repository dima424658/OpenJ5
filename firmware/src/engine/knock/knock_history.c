#include "stock_registers.h"

enum {
    CYLINDER_COUNT = 4,
    ZONES_PER_CYLINDER = 12,
    RETARD_STEP = 4,
    ZONE_HOLD_COUNT = 5,
    HOLD_CYLINDER_RETARD = 0x80,
};

static __sfr __at (0xf0) B;
static __sbit __at (0x1f) knock_fault_1f;
static __sbit __at (0x27) knock_fault_27;
static __sbit __at (0x28) knock_fault_28;
static __sbit __at (0x63) previous_fuel_mode_63;
static __sbit __at (0x65) knock_event_seen;
static __sbit __at (0xaf) interrupts_enabled;

static volatile __data __at (0x03) uint8_t bank_zero_r3;
static volatile __data __at (0x3a) uint8_t previous_knock_mask;
static volatile __data __at (0x3b) uint8_t active_knock_zone;
static volatile __data __at (0x58) uint8_t rpm_half_step_index;
static volatile __data __at (0x71) uint8_t pending_knock_mask;
static volatile __data __at (0x7b) uint8_t knock_window_override;
static volatile __idata __at (0xa9) uint8_t cylinder_retard[CYLINDER_COUNT];
static volatile __xdata __at (0xf86b) uint8_t cylinder_knock_interval[CYLINDER_COUNT];
static volatile __xdata __at (0xf900) uint8_t zone_retard_page[256];
static volatile __xdata __at (0xf932) uint8_t knock_zone;
static volatile __xdata __at (0xf953) uint8_t minimum_knock_interval;
static volatile __xdata __at (0xf954) uint8_t retard_recovery_interval;
static volatile __xdata __at (0xf955) uint8_t zone_hold_count[CYLINDER_COUNT];

extern __code const uint8_t knock_maximum_retard_raw[32];
extern volatile __code const uint8_t knock_event_masks[16];
static volatile __data __at (4) uint8_t stock_r4;

/* Both entry points share the cylinder logic. Only one keeps zone history. */
#ifndef KNOCK_CYLINDER_ONLY
#define UPDATE_KNOCK_HISTORY update_knock_history
#else
#define UPDATE_KNOCK_HISTORY update_knock_history_alternate
#endif

void UPDATE_KNOCK_HISTORY(void)
{
    if (!previous_fuel_mode_63) {
        previous_knock_mask = 0;
        for (uint8_t i = 0; i < CYLINDER_COUNT; ++i) {
            cylinder_knock_interval[i] = 0;
        }
        for (uint8_t i = 0; i < CYLINDER_COUNT; ++i) {
            cylinder_retard[i] = 0;
        }
    }

    if (knock_fault_1f || knock_fault_27 || knock_fault_28) {
        for (uint8_t i = 0; i < CYLINDER_COUNT; ++i) {
            cylinder_retard[i] = knock_maximum_retard_raw[rpm_half_step_index];
        }
        return;
    }

    knock_event_seen = 0;
    active_knock_zone = knock_zone;
    for (stock_r1 = 0; stock_r1 < CYLINDER_COUNT; ++stock_r1) {
#ifndef KNOCK_CYLINDER_ONLY
        const uint8_t zone_index = (uint8_t)(ZONES_PER_CYLINDER * stock_r1 + active_knock_zone);
        stock_r4 = zone_retard_page[zone_index];
#endif
        stock_r3 = cylinder_retard[stock_r1];

        stock_r2 = knock_event_masks[stock_r1];
        interrupts_enabled = 0;
        ACC = pending_knock_mask;
        B = ACC & stock_r2;
        pending_knock_mask = ACC ^ B;
        interrupts_enabled = 1;

        if (B != 0) {
            knock_event_seen = 1;
            if ((previous_knock_mask & stock_r2) &&
                cylinder_knock_interval[stock_r1] < minimum_knock_interval) {
                stock_r3 += RETARD_STEP;
#ifndef KNOCK_CYLINDER_ONLY
                stock_r4 += RETARD_STEP;
#endif
            }
            cylinder_knock_interval[stock_r1] = 0;
            previous_knock_mask |= stock_r2;
        } else {
            const uint8_t interval = cylinder_knock_interval[stock_r1];
            if (interval < retard_recovery_interval) {
                cylinder_knock_interval[stock_r1] = interval + 1;
            } else {
                cylinder_knock_interval[stock_r1] = 0;
                if (stock_r3 != 0) {
                    --stock_r3;
                }
#ifndef KNOCK_CYLINDER_ONLY
                if (stock_r4 != 0) {
                    --stock_r4;
                }
#endif
                previous_knock_mask &= ~stock_r2;
            }
        }

#ifndef KNOCK_CYLINDER_ONLY
        // The comparison uses the physical bank-zero R3 address.
        if (stock_r4 >= bank_zero_r3) {
            stock_r3 = stock_r4;
        } else {
            const uint8_t hold = zone_hold_count[stock_r1] - 1;
            zone_hold_count[stock_r1] = hold;
            if (hold == 0) {
                --stock_r3;
                zone_hold_count[stock_r1] = ZONE_HOLD_COUNT;
            }
        }

#endif
        B = knock_maximum_retard_raw[rpm_half_step_index];
#ifndef KNOCK_CYLINDER_ONLY
        if (stock_r4 >= B) {
            stock_r4 = B;
        }
#endif
        if (stock_r3 >= B) {
            stock_r3 = B;
        }
#ifndef KNOCK_CYLINDER_ONLY
        zone_retard_page[zone_index] = stock_r4;
#endif
        if (!(knock_window_override & HOLD_CYLINDER_RETARD)) {
            cylinder_retard[stock_r1] = stock_r3;
        }
    }
}
