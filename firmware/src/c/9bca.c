#include "stock_registers.h"

enum {
    OXYGEN_LIMIT_NEGATIVE = 0xb0,
    OXYGEN_LIMIT_POSITIVE = 0x50,
    OXYGEN_LIMIT_EVENT_COUNT = 100,
};

static __sfr __at (0xf0) B;
static __sbit __at (0x2c) oxygen_adaptation_fault_2c;
static __sbit __at (0x2d) oxygen_adaptation_fault_2d;
static __sbit __at (0x36) oxygen_feedback_tick;
static __sbit __at (0x5c) fuel_mode_flag_5c;
static __sbit __at (0x5e) purge_active;
static __sbit __at (0x5f) learning_sweep_active;
static __sbit __at (0x64) learning_purge_table_selected;
static __sbit __at (0x66) previous_oxygen_signal_rich;
static __sbit __at (0x67) oxygen_signal_rich;

static volatile __data __at (0x49) uint8_t coolant_temperature_raw;
static volatile __data __at (0x4e) uint8_t coolant_index;
static volatile __data __at (0x58) uint8_t rpm_half_bin;
static volatile __data __at (0x5d) uint8_t charge_half_bin;
static volatile __data __at (0x5f) uint8_t operating_point_index;
static volatile __data __at (0x6e) uint8_t oxygen_control_adjustment;
static volatile __data __at (0x70) uint8_t oxygen_transition_bits;

static volatile __xdata __at (0xf85b) uint8_t oxygen_center_ticks;
static volatile __xdata __at (0xf85c) uint8_t oxygen_reference_rpm;
static volatile __xdata __at (0xf85d) uint8_t oxygen_reference_charge;
static volatile __xdata __at (0xf85e) uint8_t oxygen_stable_ticks;
static volatile __xdata __at (0xf85f) uint8_t oxygen_previous_adjustment;
static volatile __xdata __at (0xf860) uint8_t oxygen_transition_delay;
static volatile __xdata __at (0xf861) uint8_t oxygen_transition_snapshot;
static volatile __xdata __at (0xf889) uint8_t learning_cell_index;
static volatile __xdata __at (0xf88c) uint8_t learning_center_correction;
static volatile __xdata __at (0xf88e) uint8_t learning_center_index;
static volatile __xdata __at (0xf898) uint8_t scheduler_tick;
static volatile __xdata __at (0xf89f) uint8_t oxygen_positive_limit_count;
static volatile __xdata __at (0xf8a6) uint8_t oxygen_negative_limit_count;
static volatile __xdata __at (0xfd00) uint8_t learned_correction_pages[2][256];

extern volatile __code const uint8_t oxygen_learning_temperature;
extern volatile __code const uint8_t oxygen_stable_cycles;
extern volatile __code const uint8_t oxygen_stationary_tolerance;
extern volatile __code const uint8_t oxygen_center_delay;
extern __code const uint8_t oxygen_center_step[39];
extern __code const uint8_t oxygen_transition_step[256];
extern __code const uint8_t oxygen_lean_rich_delay[256];
extern __code const uint8_t oxygen_rich_lean_delay[256];

extern void abs_accumulator_i8(void);
extern void reset_fuel_correction_map(void);

void track_oxygen_feedback(void);

void update_oxygen_feedback(void)
{
    if (fuel_mode_flag_5c) {
        track_oxygen_feedback();
        return;
    }

    oxygen_reference_rpm = rpm_half_bin;
    oxygen_reference_charge = charge_half_bin;
    oxygen_transition_bits = oxygen_signal_rich ? UINT8_MAX : 0;
    oxygen_center_ticks = 0;
    oxygen_stable_ticks = 0;
    oxygen_transition_delay = 0;
    oxygen_control_adjustment = 0;
    fuel_mode_flag_5c = 1;
}

void track_oxygen_feedback(void)
{
    if (oxygen_feedback_tick) {
        B = oxygen_stationary_tolerance;
        ACC = oxygen_reference_rpm - rpm_half_bin;
        abs_accumulator_i8();
        if (ACC < B) {
            ACC = oxygen_reference_charge - charge_half_bin;
            abs_accumulator_i8();
        }
        if (ACC >= B) {
            oxygen_stable_ticks = 0;
            oxygen_reference_rpm = rpm_half_bin;
            oxygen_reference_charge = charge_half_bin;
        } else if (oxygen_stable_ticks != UINT8_MAX) {
            ++oxygen_stable_ticks;
        }
    }

    if (oxygen_transition_delay != 0) {
        --oxygen_transition_delay;
    } else if (oxygen_feedback_tick && oxygen_center_ticks != UINT8_MAX) {
        ++oxygen_center_ticks;
    }

    if (!oxygen_feedback_tick) {
        return;
    }
    oxygen_feedback_tick = 0;
    if (oxygen_signal_rich == previous_oxygen_signal_rich) {
        if (oxygen_transition_delay || oxygen_center_ticks < oxygen_center_delay ||
            (scheduler_tick & 3)) {
            return;
        }

        B = oxygen_center_step[coolant_index];
        ACC = oxygen_signal_rich ? oxygen_control_adjustment - B : oxygen_control_adjustment + B;
        oxygen_control_adjustment = ACC;
        if (!(ACC & 0x80) && ACC >= OXYGEN_LIMIT_POSITIVE) {
            goto clamp_positive;
        }
        if ((ACC & 0x80) && ACC < OXYGEN_LIMIT_NEGATIVE) {
            goto clamp_negative;
        }
        return;
    }

    if (oxygen_signal_rich) {
        previous_oxygen_signal_rich = 1;
        oxygen_positive_limit_count = 0;
        oxygen_transition_delay = oxygen_rich_lean_delay[operating_point_index];
    } else {
        previous_oxygen_signal_rich = 0;
        oxygen_negative_limit_count = 0;
        oxygen_transition_delay = oxygen_lean_rich_delay[operating_point_index];
    }
    oxygen_center_ticks = 0;

    B = oxygen_transition_step[operating_point_index];
    ACC = oxygen_previous_adjustment - oxygen_control_adjustment;
    abs_accumulator_i8();
    /* DIV keeps the stock accumulator behavior even for a zero calibration. */
    __asm div ab __endasm;
    B = ACC;
    ACC = oxygen_transition_bits;
    do {
        ACC = (uint8_t)((ACC << 1) | !oxygen_signal_rich);
    } while (--B);
    oxygen_transition_bits = ACC;
    oxygen_transition_snapshot = ACC;
    oxygen_previous_adjustment = oxygen_control_adjustment;

    B = oxygen_transition_step[operating_point_index];
    ACC = oxygen_signal_rich ? oxygen_control_adjustment - B : oxygen_control_adjustment + B;
    oxygen_control_adjustment = ACC;
    if (!(ACC & 0x80) && ACC >= OXYGEN_LIMIT_POSITIVE) {
        goto clamp_positive;
    }
    if ((ACC & 0x80) && ACC < OXYGEN_LIMIT_NEGATIVE) {
        goto clamp_negative;
    }

    if (learning_sweep_active || (oxygen_transition_bits & 0x0f) != 5 ||
        coolant_temperature_raw <= oxygen_learning_temperature ||
        oxygen_stable_ticks < oxygen_stable_cycles) {
        return;
    }
    {
        volatile __xdata uint8_t *entry =
            &learned_correction_pages[learning_purge_table_selected][operating_point_index];

        ACC = *entry;
        B = oxygen_previous_adjustment;
        if (B & 0x80) {
            if (ACC != 0) {
                --ACC;
            }
        } else if (B != 0 && ACC != UINT8_MAX) {
            ++ACC;
        }
        *entry = ACC;
    }
    learning_center_correction = ACC;
    learning_center_index = operating_point_index;
    learning_purge_table_selected = purge_active;
    learning_cell_index = 0;
    learning_sweep_active = 1;
    return;

clamp_negative:
    oxygen_control_adjustment = OXYGEN_LIMIT_NEGATIVE;
    ACC = oxygen_negative_limit_count + 1;
    oxygen_negative_limit_count = ACC;
    if (ACC < OXYGEN_LIMIT_EVENT_COUNT) {
        return;
    }
    oxygen_adaptation_fault_2c = 1;
    goto restart_feedback;

clamp_positive:
    oxygen_control_adjustment = OXYGEN_LIMIT_POSITIVE;
    ACC = oxygen_positive_limit_count + 1;
    oxygen_positive_limit_count = ACC;
    if (ACC < OXYGEN_LIMIT_EVENT_COUNT) {
        return;
    }
    oxygen_adaptation_fault_2d = 1;

restart_feedback:
    oxygen_control_adjustment = 0;
    fuel_mode_flag_5c = 0;
    reset_fuel_correction_map();
}
