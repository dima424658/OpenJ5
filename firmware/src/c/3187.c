#include "stock_registers.h"

static __sfr __at (0xf0) B;
static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0xe0) ACC_BIT0;
static __sbit __at (0xe4) ACC_BIT4;
static __sbit __at (0xe5) ACC_BIT5;
static __sbit __at (0xe6) ACC_BIT6;
static __sbit __at (0xe7) ACC_BIT7;
static __sbit __at (0xf0) B_BIT0;
static __sbit __at (0xf1) B_BIT1;
static __sbit __at (0xf2) B_BIT2;
static __sbit __at (0xf3) B_BIT3;
static __sbit __at (0xf5) B_BIT5;
static __sbit __at (0xf4) B_BIT4;
static __sbit __at (0xf6) B_BIT6;
static __sbit __at (0xf7) B_BIT7;
static __sbit __at (0x50) report_misfire_in_pairs;

static volatile __data __at (0x7c) uint8_t output_flags_7c;
static volatile __xdata __at (0xfa6c) uint8_t misfire_control_fa6c;
static volatile __xdata __at (0xfa6d) uint8_t misfire_control_fa6d;
static volatile __xdata __at (0xf9b0) uint8_t retained_status;
static volatile __xdata __at (0xf9b4) uint8_t retained_misfire_flags;
static volatile __xdata __at (0xf80b) uint8_t coolant_adc_raw;
static volatile __xdata __at (0xf813) uint16_t coolant_sample_sum;
static volatile __xdata __at (0xf802) uint8_t adc5_raw;
static volatile __xdata __at (0xf819) uint16_t adc5_sample_sum;

extern void read_adc_8bit(void);
extern void accumulate_u8_xdata_u16(void);
extern void sample_ignition_supply(void);

void update_misfire_outputs(void);
void map_misfire_outputs(void);
void continue_after_misfire(void);

void misfire_event_not_pending(void)
{
    B = (misfire_control_fa6c >> 1) & 0x70;
    ACC = misfire_control_fa6d;
    B_BIT7 = ACC_BIT0;

    if (B != 0) {
        retained_status |= 0x80;
        ACC = retained_misfire_flags | B;
        if (report_misfire_in_pairs) {
            if (B_BIT4 || B_BIT6)
                ACC |= 0x50;
            if (B_BIT5 || B_BIT7)
                ACC |= 0xa0;
        }
    } else {
        retained_status &= 0x7f;
        ACC = retained_misfire_flags & 0x0f;
    }
    retained_misfire_flags = ACC;

    update_misfire_outputs();
}

void update_misfire_outputs(void)
{
    B &= (uint8_t)((misfire_control_fa6c >> 1) << 4);
    ACC = (misfire_control_fa6d & 0x0f) | B;
    B = ACC;
    if (ACC != 0 && report_misfire_in_pairs) {
        if (ACC_BIT7)
            B_BIT5 = 1;
        if (ACC_BIT6)
            B_BIT4 = 1;
        if (ACC_BIT5)
            B_BIT7 = 1;
        if (ACC_BIT4)
            B_BIT6 = 1;
    }

    map_misfire_outputs();
}

void map_misfire_outputs(void)
{
    ACC = (misfire_control_fa6d & 0x0f) | B;
    misfire_control_fa6d = ACC;
    B = 0;
    B_BIT0 = ACC_BIT4;
    B_BIT2 = ACC_BIT5;
    B_BIT3 = ACC_BIT6;
    B_BIT1 = ACC_BIT7;
    output_flags_7c |= B;
    if (B != 0)
        misfire_control_fa6d |= 0x04;

    continue_after_misfire();
}

void continue_after_misfire(void)
{
    B = 4;
    read_adc_8bit();
    coolant_adc_raw = ACC;
    STOCK_DPTR = (uint16_t)&coolant_sample_sum;
    accumulate_u8_xdata_u16();

    B = 5;
    read_adc_8bit();
    adc5_raw = ACC;
    STOCK_DPTR = (uint16_t)&adc5_sample_sum;
    accumulate_u8_xdata_u16();

    sample_ignition_supply();
}
