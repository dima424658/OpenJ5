#include "stock_registers.h"

static __sfr __at (0xf0) B;
static __sbit __at (0xd7) CY;
static volatile __xdata __at (0xfa63) uint16_t misfire_samples[4];
static volatile __xdata __at (0xfa6c) uint8_t misfire_control_fa6c;
static volatile __xdata __at (0xfa6d) uint8_t misfire_control_fa6d;
static volatile __xdata __at (0xfa70) uint16_t misfire_scaled_event;
static volatile __xdata __at (0xfa72) uint8_t misfire_counters[4];

extern __code const uint8_t misfire_counter_limit;
extern void subtract_u16_register_pairs(void);
extern void misfire_event_not_pending(void);
void publish_misfire_counter_flags(void);

void compare_misfire_samples(void)
{
    stock_r23 = misfire_scaled_event;
    stock_r01 = misfire_samples[0];
    subtract_u16_register_pairs();
    ACC = misfire_control_fa6c;
    if (CY)
        ACC &= 0xdf;
    else
        ACC |= 0x20;
    misfire_control_fa6c = ACC;

    stock_r01 = misfire_samples[1];
    subtract_u16_register_pairs();
    ACC = misfire_control_fa6c;
    if (CY)
        ACC &= 0xbf;
    else
        ACC |= 0x40;
    misfire_control_fa6c = ACC;

    stock_r01 = misfire_samples[2];
    subtract_u16_register_pairs();
    ACC = misfire_control_fa6c;
    if (CY)
        ACC &= 0x7f;
    else
        ACC |= 0x80;
    misfire_control_fa6c = ACC;

    stock_r01 = misfire_samples[3];
    subtract_u16_register_pairs();
    ACC = misfire_control_fa6d;
    if (CY)
        ACC &= 0xfe;
    else
        ACC |= 0x01;
    misfire_control_fa6d = ACC;

    publish_misfire_counter_flags();
}

void publish_misfire_counter_flags(void)
{
    B = misfire_counter_limit;
    CY = misfire_counters[0] < B;
    ACC = misfire_control_fa6c;
    if (CY)
        ACC &= 0xfd;
    else
        ACC |= 0x02;
    misfire_control_fa6c = ACC;

    CY = misfire_counters[1] < B;
    ACC = misfire_control_fa6c;
    if (CY)
        ACC &= 0xfb;
    else
        ACC |= 0x04;
    misfire_control_fa6c = ACC;

    CY = misfire_counters[2] < B;
    ACC = misfire_control_fa6c;
    if (CY)
        ACC &= 0xf7;
    else
        ACC |= 0x08;
    misfire_control_fa6c = ACC;

    CY = misfire_counters[3] < B;
    ACC = misfire_control_fa6c;
    if (CY)
        ACC &= 0xef;
    else
        ACC |= 0x10;
    misfire_control_fa6c = ACC;

    misfire_event_not_pending();
}
