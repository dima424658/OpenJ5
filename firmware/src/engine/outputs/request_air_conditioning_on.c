#include "stock_registers.h"

enum { AC_RELAY_MASK = 0x01 };

static __sfr __at (0xf0) B;
static __sbit __at (0x59) idle_regulating;
static __sbit __at (0x6e) air_conditioning_active;
static __sbit __at (0xe0) ACC_BIT0;

static volatile __data __at (0x49) uint8_t coolant_temperature_raw;
static volatile __data __at (0x64) uint8_t iac_target;
static volatile __xdata __at (0xf84f) uint8_t idle_rpm_correction;
static volatile __xdata __at (0xf870) uint8_t ac_throttle_off_ticks;
static volatile __xdata __at (0xf871) uint8_t ac_idle_delay_ticks;
static volatile __xdata __at (0xf8ae) uint8_t vehicle_speed;
static volatile __xdata __at (0xf999) uint8_t relay_output_latch;
static volatile __xdata __at (0xfa77) uint8_t ac_release_seconds;

extern volatile __code const uint8_t ac_initial_idle_rpm_offset;
extern volatile __code const uint8_t ac_hot_iac_extra_steps;
extern volatile __code const uint8_t ac_hot_iac_threshold;
extern volatile __code const uint8_t ac_iac_offset_steps;
extern volatile __code const uint8_t ac_idle_adaptation_delay;
extern volatile __code const uint8_t ac_engage_speed_threshold;

extern void clear_iac_history(void);
extern void continue_after_ac_control(void);

void prepare_ac_idle_compensation(void);
void engage_air_conditioning(void);

void request_air_conditioning_on(void)
{
    ACC = relay_output_latch;
    if (ACC_BIT0) {
        continue_after_ac_control();
        return;
    }

    ac_throttle_off_ticks = 0;
    ac_release_seconds = 0;
    prepare_ac_idle_compensation();
}

void prepare_ac_idle_compensation(void)
{
    if (idle_regulating) {
        B = ac_idle_adaptation_delay;
        ACC = ac_idle_delay_ticks;
        if (ACC == 0) {
            iac_target += ac_iac_offset_steps;
            if (ac_hot_iac_threshold < coolant_temperature_raw) {
                iac_target += ac_hot_iac_extra_steps;
            }
            idle_rpm_correction = ac_initial_idle_rpm_offset;
            clear_iac_history();
            ACC = 0;
        }

        ++ACC;
        ac_idle_delay_ticks = ACC;
        if (ACC < B) {
            continue_after_ac_control();
            return;
        }
    }
    engage_air_conditioning();
}

void engage_air_conditioning(void)
{
    if (!idle_regulating) {
        B = ac_engage_speed_threshold;
        if (vehicle_speed < B) {
            continue_after_ac_control();
            return;
        }
    }

    ac_idle_delay_ticks = 0;
    relay_output_latch |= AC_RELAY_MASK;
    air_conditioning_active = 1;
    continue_after_ac_control();
}
