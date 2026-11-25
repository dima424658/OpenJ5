#include "stock_registers.h"

enum {
    AC_OVERRIDE = 0x04,
    AC_RELAY_MASK = 0x01,
    AC_REQUEST_ADC_CHANNEL = 8,
};

static __sfr __at (0xf0) B;
static __sbit __at (0xd7) CY;
static __sbit __at (0x49) tick_1s;
static __sbit __at (0x6e) air_conditioning_active;
static __sbit __at (0x72) ac_override_on;

static volatile __data __at (0x49) uint8_t coolant_temperature_raw;
static volatile __data __at (0x52) uint8_t throttle_position;
static volatile __data __at (0x79) uint8_t output_control_79;
static volatile __xdata __at (0xf80d) uint8_t ignition_supply_raw;
static volatile __xdata __at (0xf84f) uint8_t idle_rpm_correction;
static volatile __xdata __at (0xf86f) uint8_t ac_request_history;
static volatile __xdata __at (0xf870) uint8_t ac_throttle_off_ticks;
static volatile __xdata __at (0xf871) uint8_t ac_idle_delay_ticks;
static volatile __xdata __at (0xf873) uint8_t time_since_start_f873;
static volatile __xdata __at (0xf8ae) uint8_t vehicle_speed;
static volatile __xdata __at (0xf999) uint8_t relay_output_latch;
static volatile __xdata __at (0xfa77) uint8_t ac_release_seconds;

extern volatile __code const uint8_t oxygen_heater_supply_cutoff;
extern volatile __code const uint8_t ac_coolant_enable_limit;
extern volatile __code const uint8_t ac_coolant_disable_limit;
extern volatile __code const uint8_t ac_start_delay_ticks;
extern volatile __code const uint8_t ac_throttle_enable_limit;
extern volatile __code const uint8_t ac_throttle_disable_limit;
extern volatile __code const uint8_t ac_throttle_off_delay_ticks;
extern volatile __code const uint8_t ac_release_speed_threshold;
extern volatile __code const uint8_t ac_release_seconds_limit;

extern void read_adc_8bit(void);
extern void request_air_conditioning_on(void);
extern void prepare_ac_idle_compensation(void);
extern void continue_after_ac_control(void);

void disable_air_conditioning(void);
void check_ac_start_and_throttle(void);
void skip_to_ac_disable(void);

void cooling_fan_continue(void)
{
    if (output_control_79 & AC_OVERRIDE) {
        CY = ac_override_on;
        if (!CY) {
            skip_to_ac_disable();
            return;
        }
        request_air_conditioning_on();
        return;
    }

    B = oxygen_heater_supply_cutoff;
    if (ignition_supply_raw >= B) {
        disable_air_conditioning();
        return;
    }
    if (air_conditioning_active) {
        B = ac_coolant_disable_limit;
    } else {
        B = ac_coolant_enable_limit;
    }
    if (coolant_temperature_raw >= B) {
        disable_air_conditioning();
        return;
    }

    B = AC_REQUEST_ADC_CHANNEL;
    read_adc_8bit();
    CY = !ACC_SIGN;
    ACC = (ac_request_history << 1) | CY;
    ac_request_history = ACC;
    if (ACC == 0) {
        check_ac_start_and_throttle();
        return;
    }
    if (ACC == UINT8_MAX) {
        disable_air_conditioning();
        return;
    }
    CY = air_conditioning_active;
    if (CY) {
        check_ac_start_and_throttle();
        return;
    }
    skip_to_ac_disable();
}

void skip_to_ac_disable(void)
{
    disable_air_conditioning();
}

void check_ac_start_and_throttle(void)
{
    B = ac_start_delay_ticks;
    if (time_since_start_f873 < B) {
        disable_air_conditioning();
        return;
    }
    CY = air_conditioning_active;
    if (CY) {
        B = ac_throttle_disable_limit;
    } else {
        B = ac_throttle_enable_limit;
    }
    if (throttle_position < B) {
        request_air_conditioning_on();
        return;
    }

    B = ac_throttle_off_delay_ticks;
    ACC = ac_throttle_off_ticks;
    if (ACC < B) {
        ac_throttle_off_ticks = ACC + 1;
        prepare_ac_idle_compensation();
        return;
    }
    disable_air_conditioning();
}

void disable_air_conditioning(void)
{
    ACC = relay_output_latch;
    if ((ACC & AC_RELAY_MASK) || air_conditioning_active) {
        relay_output_latch = ACC & (uint8_t)~AC_RELAY_MASK;
        ac_idle_delay_ticks = 0;
        idle_rpm_correction = 0;

        B = ac_release_speed_threshold;
        if (vehicle_speed >= B) {
            goto release;
        }
        if (tick_1s) {
            B = ac_release_seconds_limit;
            ACC = ac_release_seconds + 1;
            ac_release_seconds = ACC;
            if (ACC >= B) {
release:
                air_conditioning_active = 0;
                ac_release_seconds = 0;
            }
        }
    }
    continue_after_ac_control();
}
