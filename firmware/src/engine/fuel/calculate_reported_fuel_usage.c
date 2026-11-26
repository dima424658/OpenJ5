#include "stock_registers.h"

enum {
    MIXTURE_DIVISOR_BIAS = 128,
    INJECTION_CORRECTION_HIGH_BIT = 0x80,
    FUEL_USAGE_SCALE = 233,
};

static __sfr __at (0xf0) B;
static __sbit __at (0x44) mdu_interrupted;
static __sbit __at (0x5b) deceleration_fuel_cut;
static __sbit __at (0xf7) B_SIGN;
static __sfr __at (0xe9) mdu_0;
static __sfr __at (0xea) mdu_1;
static __sfr __at (0xeb) mdu_2;
static __sfr __at (0xec) mdu_3;
static __sfr __at (0xed) mdu_4;
static __sfr __at (0xee) mdu_5;

static volatile __data __at (0x00) uint8_t bank_zero_r0;
static volatile __data __at (0x01) uint8_t bank_zero_r1;
static volatile __data __at (0x04) uint8_t stock_r4;
static volatile __data __at (0x55) uint8_t rpm_eighth;
static volatile __data __at (0x61) uint8_t applied_mixture;
static volatile __data __at (0x6f) uint8_t injection_time_correction;
static volatile __xdata __at (0xf82e) uint16_t air_charge;
static volatile __xdata __at (0xf862) uint16_t reported_fuel_rate;
static volatile __xdata __at (0xf874) uint16_t reported_fuel_per_distance;
static volatile __xdata __at (0xf8ae) uint8_t vehicle_speed;

extern void multiply_stock_u16_u8(void);
extern void add_u16_register_pairs(void);
extern void update_vehicle_speed(void);

void calculate_fuel_per_distance(void);
void mark_fuel_distance_unavailable(void);
void publish_fuel_per_distance(void);

void calculate_reported_fuel_usage(void)
{
    // The first quotient is truncated to a word before applying the correction.
    do {
        mdu_interrupted = 0;
        stock_r01 = air_charge;
        B = rpm_eighth;
        multiply_stock_u16_u8();

        const uint16_t divisor = MIXTURE_DIVISOR_BIAS + applied_mixture;
        stock_r2 = (uint8_t)divisor;
        mdu_0 = B;
        mdu_1 = stock_r0;
        mdu_2 = stock_r1;
        mdu_3 = 0;
        mdu_4 = stock_r2;
        mdu_5 = divisor >> 8;
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
        ACC = mdu_2;
        ACC = mdu_3;
        ACC = mdu_4;
        ACC = mdu_5;
    } while (mdu_interrupted);

    B = injection_time_correction;
    // The stock addition sources use physical bank-zero addresses.
    stock_r2 = bank_zero_r0;
    stock_r3 = bank_zero_r1;
    B_SIGN = !B_SIGN;
    multiply_stock_u16_u8();
    ACC = injection_time_correction;
    if (ACC_SIGN) {
        add_u16_register_pairs();
    }

    B = FUEL_USAGE_SCALE;
    multiply_stock_u16_u8();
    stock_r4 = B;
    if (deceleration_fuel_cut) {
        stock_r01 = 0;
        stock_r4 = 0;
    }
    reported_fuel_rate = stock_r01;
    calculate_fuel_per_distance();
}

void calculate_fuel_per_distance(void)
{
    ACC = vehicle_speed;
    B = ACC;
    if (ACC == 0) {
        mark_fuel_distance_unavailable();
        return;
    }

    do {
        mdu_interrupted = 0;
        stock_r01 = reported_fuel_rate;
        mdu_0 = stock_r4;
        mdu_1 = stock_r0;
        mdu_2 = stock_r1;
        mdu_3 = 0;
        mdu_4 = B;
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
        stock_r2 = mdu_2;
        stock_r3 = mdu_3;
        ACC = mdu_4;
        ACC = mdu_5;
    } while (mdu_interrupted);

    if (stock_r2 != 0 || stock_r3 != 0) {
        mark_fuel_distance_unavailable();
        return;
    }
    publish_fuel_per_distance();
}

void mark_fuel_distance_unavailable(void)
{
    stock_r01 = UINT16_MAX;
    publish_fuel_per_distance();
}

void publish_fuel_per_distance(void)
{
    reported_fuel_per_distance = stock_r01;
    update_vehicle_speed();
}
