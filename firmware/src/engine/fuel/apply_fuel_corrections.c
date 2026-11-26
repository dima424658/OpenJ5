#include "stock_registers.h"

static __sfr __at (0xe9) mdu_0;
static __sfr __at (0xea) mdu_1;
static __sfr __at (0xeb) mdu_2;
static __sfr __at (0xec) mdu_3;
static __sfr __at (0xed) mdu_4;
static __sfr __at (0xee) mdu_5;
static __sfr __at (0xf0) B;
static __sbit __at (0xd7) CY;
static __sbit __at (0x50) split_injection_dose;

static volatile __data __at (0x08) uint16_t bank_one_r01;
static volatile __data __at (0x0a) uint16_t bank_one_r23;
static volatile __data __at (0x50) uint8_t injector_voltage_offset;
static volatile __data __at (0x6f) uint8_t injection_time_correction;
static volatile __data __at (0x77) uint8_t idle_injection_correction;
static volatile __xdata __at (0xf841) uint16_t final_injection_dose;
static volatile __xdata __at (0xf987) uint16_t delivered_cyclic_fuel;

extern volatile __code const uint8_t injector_static_factor;
extern volatile __code const uint8_t minimum_fuel_dose[2];

extern void shift_right_u16_1(void);
extern void multiply_stock_u16_u8(void);
extern void add_u16_register_pairs(void);
extern void schedule_injector_pulses(void);
extern void continue_after_injection(void);

void apply_minimum_fuel_dose(void);
void publish_cyclic_fuel(void);

#define APPLY_INJECTION_CORRECTION(correction) \
    do {                                       \
        bank_one_r23 = bank_one_r01;           \
        B = (correction) ^ 0x80;               \
        multiply_stock_u16_u8();               \
        if ((correction) & 0x80) {             \
            add_u16_register_pairs();          \
            if (CY) {                          \
                stock_r01 = UINT16_MAX;        \
            }                                  \
        }                                      \
    } while (0)

void apply_fuel_corrections(void)
{
    if (split_injection_dose) {
        shift_right_u16_1();
    }
    APPLY_INJECTION_CORRECTION(injection_time_correction);
    APPLY_INJECTION_CORRECTION(idle_injection_correction);

    B = injector_static_factor;
    multiply_stock_u16_u8();
    delivered_cyclic_fuel = stock_r01;
    stock_r23 = injector_voltage_offset;
    add_u16_register_pairs();
    apply_minimum_fuel_dose();
}

void apply_minimum_fuel_dose(void)
{
    stock_r2 = minimum_fuel_dose[0];
    stock_r3 = minimum_fuel_dose[1];
    if (stock_r01 < stock_r23) {
        stock_r01 = stock_r23;
    }
    publish_cyclic_fuel();
}

void publish_cyclic_fuel(void)
{
    final_injection_dose = stock_r01;
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

    if (stock_r01 == 0) {
        continue_after_injection();
        return;
    }
    schedule_injector_pulses();
}
