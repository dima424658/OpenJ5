#include "stock_registers.h"

enum {
    MIN_ADAPTATION_READY_ERROR = 20,
    ADAPTATION_READY_COUNT = 12,
};

static __sfr __at (0xf0) B;
static __sbit __at (0x16) mode_flag_16;
static __sbit __at (0x2f) idle_adaptation_ready_2f;
static __sbit __at (0x62) idle_transition_hold_62;
static volatile __data __at (0x49) uint8_t coolant_temperature_raw;
static volatile __data __at (0x53) uint8_t engine_rpm_10;
static volatile __data __at (0x6d) int8_t idle_rpm_error;
static volatile __xdata __at (0xf847) uint8_t idle_transition_rpm_1;
static volatile __xdata __at (0xf8ae) uint8_t vehicle_speed;
static volatile __xdata __at (0xf8c1) uint8_t hold_counter_f8c1;
static volatile __xdata __at (0xf986) uint8_t idle_adaptation_count_f986;

extern volatile __code const uint8_t fuel_cut_coolant_threshold;
extern void abs_accumulator_i8(void);
extern void clear_iac_history(void);

void update_idle_transition_hold(void);
void finish_iac_adaptation_window(void);

void update_iac_adaptation_ready(void)
{
    B = fuel_cut_coolant_threshold;
    if (coolant_temperature_raw < B || vehicle_speed != 0) {
        goto reset_count;
    }
    ACC = (uint8_t)idle_rpm_error;
    abs_accumulator_i8();
    if (ACC < MIN_ADAPTATION_READY_ERROR) {
        goto reset_count;
    }

    ACC = idle_adaptation_count_f986 + 1;
    if (ACC != 0) {
        idle_adaptation_count_f986 = ACC;
    }
    if (ACC >= ADAPTATION_READY_COUNT) {
        idle_adaptation_ready_2f = 1;
    }
    update_idle_transition_hold();
    return;

reset_count:
    idle_adaptation_ready_2f = 0;
    idle_adaptation_count_f986 = 0;
    update_idle_transition_hold();
}

void update_idle_transition_hold(void)
{
    B = idle_transition_rpm_1;
    if (engine_rpm_10 < B) {
        idle_transition_hold_62 = 0;
        hold_counter_f8c1 = 0;
        mode_flag_16 = 0;
    }
    finish_iac_adaptation_window();
}

void finish_iac_adaptation_window(void)
{
    clear_iac_history();
}
