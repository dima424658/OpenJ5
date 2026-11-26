#include "injector_timer.h"

static __sfr __at (0xe9) mdu_0;
static __sfr __at (0xea) mdu_1;
static __sfr __at (0xeb) mdu_2;
static __sfr __at (0xec) mdu_3;
static __sfr __at (0xed) mdu_4;
static __sfr __at (0xee) mdu_5;
static __sfr __at (0xf0) B;

static __sbit __at (0x05) fuel_configuration_flag_05;
static __sbit __at (0x0e) asynchronous_enrichment_enabled;
static __sbit __at (0x14) injection_selection_flag_14;
static __sbit __at (0x45) output_state_flag_45;
static __sbit __at (0x50) split_injection_dose;
static __sbit __at (0x53) startup_flag_53;
static __sbit __at (0xd7) CY;

static volatile __data __at (0x08) uint16_t bank_one_r01;
static volatile __data __at (0x0a) uint16_t bank_one_r23;
static volatile __data __at (0x50) uint8_t injector_voltage_offset;
static volatile __data __at (0x6f) uint8_t injection_time_correction;
static volatile __data __at (0x7c) uint8_t output_flags_7c;
static volatile __xdata __at (0xf82e) uint16_t cylinder_charge;
static volatile __xdata __at (0xf87f) uint8_t enrichment_flags;
static volatile __xdata __at (0xf8f5) uint8_t cylinder_charge_sum[3];
static volatile __xdata __at (0xf97b) uint16_t opening_fuel_delta;
static volatile __xdata __at (0xf993) uint16_t asynchronous_fuel_dose;

extern volatile __code const uint8_t injector_static_factor;

extern void shift_right_u16_1(void);
extern void add_u16_register_pairs(void);
extern void multiply_stock_u16_u8(void);
extern void finish_crank_without_fuel(void);

void calculate_async_enrichment(void);
void schedule_async_enrichment(void);

void continue_after_injection(void)
{
    if (!startup_flag_53) {
        stock_r01 = cylinder_charge;
        const uint16_t prior_low = ((uint16_t)cylinder_charge_sum[1] << 8) |
            cylinder_charge_sum[0];
        const uint32_t low_sum = (uint32_t)prior_low + stock_r01;
        cylinder_charge_sum[0] = (uint8_t)low_sum;
        cylinder_charge_sum[1] = (uint8_t)(low_sum >> 8);
        // The low word wraps even when the high byte is already 255.
        const uint16_t high_sum = cylinder_charge_sum[2] + (low_sum >> 16);
        if (high_sum <= UINT8_MAX) {
            cylinder_charge_sum[2] = (uint8_t)high_sum;
        }
    }
    if (!asynchronous_enrichment_enabled) {
        finish_crank_without_fuel();
        return;
    }
    if (output_state_flag_45) {
        asynchronous_fuel_dose = 0;
        finish_crank_without_fuel();
        return;
    }
    calculate_async_enrichment();
}

void calculate_async_enrichment(void)
{
    stock_r01 = opening_fuel_delta;
    if (stock_r01 == 0) {
        asynchronous_fuel_dose = 0;
        enrichment_flags &= 0xfe;
        schedule_async_enrichment();
        return;
    }

    enrichment_flags |= 1;
    if (fuel_configuration_flag_05) {
        split_injection_dose = injection_selection_flag_14;
    } else {
        split_injection_dose = 1;
    }
    if (split_injection_dose) {
        shift_right_u16_1();
    }
    bank_one_r23 = bank_one_r01;
    B = injection_time_correction ^ 0x80;
    multiply_stock_u16_u8();
    if (injection_time_correction & 0x80) {
        add_u16_register_pairs();
        if (CY) {
            stock_r01 = UINT16_MAX;
        }
    }

    B = injector_static_factor;
    multiply_stock_u16_u8();
    stock_r23 = injector_voltage_offset;
    add_u16_register_pairs();
    asynchronous_fuel_dose = stock_r01;

    B = 16;
    multiply_stock_u16_u8();
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
    stock_r0 = mdu_0;
    stock_r1 = mdu_1;
    (void)mdu_2;
    (void)mdu_3;
    (void)mdu_4;
    (void)mdu_5;
    schedule_async_enrichment();
}

#define EXTEND_ACTIVE_INJECTOR(channel, output, compare_low, compare_high)    \
    do {                                                                      \
        const uint8_t mask = 1u << (channel);                                 \
        if (!(output_flags_7c & mask) && (output)) {                          \
            START_INJECTOR_PULSE(mask, 0, output, compare_low, compare_high); \
        }                                                                     \
    } while (0)

void schedule_async_enrichment(void)
{
    if (stock_r01 != 0) {
        EXTEND_ACTIVE_INJECTOR(0, injector_output_0, CML0, CMH0);
        EXTEND_ACTIVE_INJECTOR(1, injector_output_1, CML1, CMH1);
        EXTEND_ACTIVE_INJECTOR(2, injector_output_2, CML2, CMH2);
        EXTEND_ACTIVE_INJECTOR(3, injector_output_3, CML3, CMH3);
    }
    finish_crank_without_fuel();
}
