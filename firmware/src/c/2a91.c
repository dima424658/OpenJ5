#include "stock_registers.h"

enum {
    CRANK_STOP_TICKS = 22,
    CRANK_RESUME_TICKS = 11,
};

static __sfr __at (0xa5) sfr_a5;
static __sfr __at (0xa6) sfr_a6;
static __sfr __at (0xf0) B;
static __sbit __at (0x31) event_flag_31;
static __sbit __at (0x35) crank_event;
static __sbit __at (0x58) crank_stopped;
static __sbit __at (0xaf) interrupts_enabled;
static __sbit __at (0xf9) output_control_f9;
static __sbit __at (0xfa) output_control_fa;

static volatile __data __at (0x38) uint8_t crank_period_low;
static volatile __data __at (0x39) uint8_t crank_period_high;
static volatile __data __at (0x49) uint8_t coolant_temperature_raw;
static volatile __data __at (0x53) uint8_t engine_rpm_10;
static volatile __data __at (0x54) uint8_t rpm_state_54;
static volatile __data __at (0x55) uint8_t rpm_state_55;
static volatile __data __at (0x57) uint8_t rpm_axis_index_q4;
static volatile __data __at (0x58) uint8_t rpm_half_step_index;
static volatile __data __at (0x5a) uint8_t ticks_since_crank_event;
static volatile __xdata __at (0xf938) uint8_t dynamic_ignition_retard;
static volatile __xdata __at (0xf97b) uint16_t dynamic_fuel_decay;

extern volatile __code const uint8_t dynamic_ignition_temp_limit;
extern volatile __code const uint8_t dynamic_ignition_gain;
extern volatile __code const uint8_t dynamic_ignition_max_retard;

extern void multiply_stock_u16_u8(void);
extern void rpm_update_continued(void);
extern void calculate_rpm_from_period(void);
void check_crank_activity(void);
void sample_crank_period(void);

void continue_cycle_fuel(void)
{
    if (dynamic_ignition_temp_limit < coolant_temperature_raw) {
        stock_r01 = dynamic_fuel_decay;
        B = dynamic_ignition_gain;
        multiply_stock_u16_u8();
        if (stock_r1 != 0) {
            stock_r0 = UINT8_MAX;
        }
        B = dynamic_ignition_max_retard;
        ACC = stock_r0;
        if (ACC >= B) {
            ACC = B;
        }
    } else {
        ACC = 0;
    }
    dynamic_ignition_retard = ACC;
    check_crank_activity();
}

void check_crank_activity(void)
{
    if (crank_event) {
        crank_event = 0;
        if (crank_stopped && ticks_since_crank_event < CRANK_RESUME_TICKS) {
            crank_stopped = 0;
        }
        ticks_since_crank_event = 0;
    } else {
        ACC = ticks_since_crank_event + 1;
        if (ACC != 0) {
            ticks_since_crank_event = ACC;
        }
        if (!crank_stopped && ticks_since_crank_event >= CRANK_STOP_TICKS) {
            crank_stopped = 1;
            event_flag_31 = 0;
            engine_rpm_10 = 0;
            rpm_state_54 = 0;
            rpm_state_55 = 0;
            rpm_axis_index_q4 = 0;
            rpm_half_step_index = 0;
            sfr_a5 |= 0x06;
            output_control_f9 = 1;
            output_control_fa = 1;
            sfr_a6 &= 0xf9;
            rpm_update_continued();
            return;
        }
    }
    sample_crank_period();
}

void sample_crank_period(void)
{
    interrupts_enabled = 0;
    stock_r2 = crank_period_low;
    stock_r3 = crank_period_high;
    interrupts_enabled = 1;
    if ((stock_r2 | stock_r3) == 0) {
        rpm_update_continued();
        return;
    }
    calculate_rpm_from_period();
}
