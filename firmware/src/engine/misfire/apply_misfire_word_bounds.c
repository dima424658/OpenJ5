#include "stock_registers.h"

static __sfr __at (0xf0) B;
static __sbit __at (0xd7) CY;
static __sbit __at (0xe0) ACC_BIT0;
static __sbit __at (0xe1) ACC_BIT1;
static __sbit __at (0xe2) ACC_BIT2;
static __sbit __at (0xe3) ACC_BIT3;
static __sbit __at (0xe4) ACC_BIT4;
static __sbit __at (0xe5) ACC_BIT5;
static __sbit __at (0xe6) ACC_BIT6;
static __sbit __at (0x5b) deceleration_fuel_cut;
static volatile __data __at (4) uint8_t stock_r4;
static volatile __data __at (0x55) uint8_t rpm_eighth;
static volatile __data __at (0x58) uint8_t rpm_nearest_half_bin;
static volatile __xdata __at (0xfa17) uint16_t misfire_bounded_word;
static volatile __xdata __at (0xf93d) uint8_t misfire_conditions;
static volatile __xdata __at (0xf94a) uint8_t misfire_sample_f94a;
static volatile __xdata __at (0xfa5e) uint8_t rpm_eighth_increase;
static volatile __xdata __at (0xf82e) uint16_t misfire_input_f82e;
static volatile __xdata __at (0xf9be) uint8_t elapsed_seconds_f9be;
static volatile __xdata __at (0xfa5f) uint8_t misfire_ready_cycles;

enum { CONDITIONS_MASK = 0x7f, INHIBITED = 0x80 };

extern __code const uint8_t misfire_rpm_limits[2];
extern __code const uint8_t misfire_input_limits[3];
extern __code const uint8_t misfire_ready_delay;
extern __code const uint8_t misfire_word_limits[4];
extern __code const uint8_t misfire_min_input_by_rpm[32];
extern void minimum_u16_register_pair(void);
extern void maximum_u16_register_pair(void);
extern void shift_right_u16_3(void);
extern void update_secondary_misfire(void);

void evaluate_misfire_conditions(void);
void publish_misfire_conditions(void);

void apply_misfire_word_bounds(void)
{
    stock_r2 = misfire_word_limits[0];
    stock_r3 = misfire_word_limits[1];
    minimum_u16_register_pair();
    stock_r2 = misfire_word_limits[2];
    stock_r3 = misfire_word_limits[3];
    maximum_u16_register_pair();
    misfire_bounded_word = stock_r01;
    evaluate_misfire_conditions();
}

void evaluate_misfire_conditions(void)
{
    stock_r4 = misfire_conditions;
    CY = misfire_rpm_limits[0] < rpm_eighth;
    ACC = stock_r4;
    ACC_BIT0 = CY;
    stock_r4 = ACC;
    CY = misfire_rpm_limits[1] >= rpm_eighth;
    ACC = stock_r4;
    ACC_BIT1 = CY;
    stock_r4 = ACC;
    B = misfire_input_limits[1];
    CY = misfire_sample_f94a < B;
    ACC = stock_r4;
    ACC_BIT2 = CY;
    stock_r4 = ACC;
    B = misfire_input_limits[0];
    CY = rpm_eighth_increase < B;
    ACC = stock_r4;
    ACC_BIT3 = CY;
    stock_r4 = ACC;

    B = misfire_min_input_by_rpm[rpm_nearest_half_bin];
    stock_r01 = misfire_input_f82e;
    shift_right_u16_3();
    if (stock_r1 != 0)
        stock_r0 = UINT8_MAX;
    CY = stock_r0 >= B;
    ACC = stock_r4;
    ACC_BIT4 = CY;
    ACC_BIT5 = !deceleration_fuel_cut;
    stock_r4 = ACC;

    B = misfire_input_limits[2];
    CY = elapsed_seconds_f9be >= B;
    ACC = stock_r4;
    ACC_BIT6 = CY;
    stock_r4 = ACC;
    publish_misfire_conditions();
}

void publish_misfire_conditions(void)
{
    if ((ACC & CONDITIONS_MASK) == CONDITIONS_MASK) {
        ACC = misfire_ready_cycles + 1;
        if (ACC == 0)
            ACC = UINT8_MAX;
        misfire_ready_cycles = ACC;
        B = ACC;
        ACC = stock_r4;
        if (ACC_SIGN && misfire_ready_delay < B)
            misfire_conditions = stock_r4 & (uint8_t)~INHIBITED;
    } else {
        misfire_ready_cycles = 0;
        misfire_conditions = stock_r4 | INHIBITED;
    }
    update_secondary_misfire();
}
