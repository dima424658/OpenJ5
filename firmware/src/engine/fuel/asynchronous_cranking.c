#include "stock_registers.h"

static __sfr __at (0xc2) compare_base_low;
static __sfr __at (0xc3) compare_base_high;
static __sfr __at (0xd2) injector_compare_1_low;
static __sfr __at (0xd3) injector_compare_1_high;
static __sfr __at (0xd4) injector_compare_2_low;
static __sfr __at (0xd5) injector_compare_2_high;
static __sfr __at (0xd6) injector_compare_3_low;
static __sfr __at (0xd7) injector_compare_3_high;
static __sfr __at (0xe2) injector_compare_4_low;
static __sfr __at (0xe3) injector_compare_4_high;
static __sfr __at (0xe8) P4;
static __sfr __at (0xe9) mdu_0;
static __sfr __at (0xea) mdu_1;
static __sfr __at (0xeb) mdu_2;
static __sfr __at (0xec) mdu_3;
static __sfr __at (0xed) mdu_4;
static __sfr __at (0xee) mdu_5;
static __sfr __at (0xf0) B;
static __sfr __at (0xf6) sfr_f6;

static __sbit __at (0x0a) async_fuel_on_repeat_start;
static __sbit __at (0x18) ignition_supply_low;
static __sbit __at (0x31) event_flag_31;
static __sbit __at (0x44) mdu_interrupted;
static __sbit __at (0x51) startup_flag_51;
static __sbit __at (0x53) startup_flag_53;
static __sbit __at (0x57) async_cranking_fuel_done;
static __sbit __at (0x73) service_pump_output;
static __sbit __at (0xaf) EA;
static __sbit __at (0xd7) CY;

static volatile __data __at (0x4e) uint8_t coolant_nearest_index;
static volatile __data __at (0x50) uint8_t injector_voltage_offset;
static volatile __data __at (0x52) uint8_t throttle_position;
static volatile __data __at (0x53) uint8_t engine_rpm_10;
static volatile __data __at (0x6f) uint8_t fuel_correction;
static volatile __data __at (0x79) uint8_t output_override_flags;
static volatile __data __at (0x7c) uint8_t output_mask_7c;

static volatile __xdata __at (0xf829) uint8_t cranking_cycle_index;
static volatile __xdata __at (0xf872) uint8_t start_exit_pending;
static volatile __xdata __at (0xf873) uint8_t time_since_start_f873;
static volatile __xdata __at (0xf999) uint8_t relay_output_latch;

extern volatile __code const uint8_t injector_static_factor;
extern volatile __code const uint8_t cranking_sync_delay;
extern volatile __code const uint8_t startup_exit_rpm_low;
extern __code const uint8_t throttle_quantization_q4[256];
extern __code const uint8_t cranking_throttle_correction[16];
extern __code const uint8_t cranking_async_fuel_raw[39];
extern __code const uint8_t startup_exit_rpm_by_coolant[39];

extern void multiply_stock_u16_u8(void);
extern void add_u16_register_pairs(void);
extern void apply_cranking_outputs(void);
extern void select_knock_regulation_zone(void);

void skip_async_cranking(void);
void finish_async_cranking(void);
void advance_cranking_exit(void);

void cancel_async_cranking(void)
{
    start_exit_pending = 0;
    skip_async_cranking();
}

void skip_async_cranking(void)
{
    finish_async_cranking();
}

void begin_asynchronous_cranking(void)
{
    const uint8_t pump_output = !(output_override_flags & 8) || service_pump_output;
    relay_output_latch = (relay_output_latch & 0xbf) | (pump_output ? 0x40 : 0);
    if (start_exit_pending) {
        skip_async_cranking();
        return;
    }
    if (ignition_supply_low || async_cranking_fuel_done) {
        cancel_async_cranking();
        return;
    }

    const uint8_t throttle_bin = ((uint8_t)(throttle_quantization_q4[throttle_position] + 8)) >> 4;
    B = cranking_throttle_correction[throttle_bin];
    ACC = ((uint16_t)cranking_async_fuel_raw[coolant_nearest_index] * B) >> 8;
    start_exit_pending = ACC;
    if (!ACC) {
        cancel_async_cranking();
        return;
    }

    stock_r1 = ACC;
    stock_r0 = 0;
    stock_r3 = ACC;
    stock_r2 = 0;
    B = fuel_correction ^ 0x80;
    multiply_stock_u16_u8();
    if (fuel_correction & 0x80) {
        add_u16_register_pairs();
        if (CY) {
            stock_r01 = UINT16_MAX;
        }
    }

    B = injector_static_factor;
    multiply_stock_u16_u8();
    stock_r23 = injector_voltage_offset;
    add_u16_register_pairs();
    B = 16;
    multiply_stock_u16_u8();

    /* Convert the scaled pulse width to the timer's compare units. */
    do {
        mdu_interrupted = 0;
        mdu_0 = B;
        mdu_1 = stock_r0;
        mdu_2 = stock_r1;
        mdu_3 = 0;
        mdu_4 = 12;
        mdu_5 = 0;
        __asm
            nop
            nop
            nop
            nop
            nop
            nop
        __endasm;
        stock_r2 = mdu_0;
        stock_r3 = mdu_1;
        ACC = mdu_2;
        ACC = mdu_3;
        ACC = mdu_4;
        ACC = mdu_5;
    } while (mdu_interrupted);

    sfr_f6 &= 0xf0;
    EA = 0;
    ACC = output_mask_7c & 0x0f;
    if (!ACC) {
        P4 &= 0xf0;
    }
    compare_base_low = ACC;
    EA = 1;
    stock_r23 += (uint16_t)compare_base_low | ((uint16_t)compare_base_high << 8);
    injector_compare_1_high = stock_r3;
    injector_compare_2_high = stock_r3;
    injector_compare_3_high = stock_r3;
    injector_compare_4_high = stock_r3;
    injector_compare_1_low = stock_r2;
    injector_compare_2_low = stock_r2;
    injector_compare_3_low = stock_r2;
    injector_compare_4_low = stock_r2;

    sfr_f6 |= 0x0f;
    P4 |= 0x0f;
    finish_async_cranking();
}

void finish_async_cranking(void)
{
    if (!async_fuel_on_repeat_start) {
        async_cranking_fuel_done = 1;
    }
    cranking_cycle_index = 0;
    time_since_start_f873 = 0;
    advance_cranking_exit();
}

void advance_cranking_exit(void)
{
    ACC = time_since_start_f873 + 1;
    if (ACC) {
        time_since_start_f873 = ACC;
    }
    if (time_since_start_f873 >= cranking_sync_delay) {
        event_flag_31 = 1;
    }
    if (startup_flag_53) {
        B = startup_exit_rpm_by_coolant[coolant_nearest_index];
        if (engine_rpm_10 < B) {
            startup_flag_51 = engine_rpm_10 < startup_exit_rpm_low;
            if (!startup_flag_51) {
                start_exit_pending = 0;
            }
            apply_cranking_outputs();
            return;
        }
        startup_flag_53 = 0;
        startup_flag_51 = 0;
    }
    select_knock_regulation_zone();
}
