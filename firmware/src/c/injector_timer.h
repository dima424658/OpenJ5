#ifndef JANVAR_INJECTOR_TIMER_H
#define JANVAR_INJECTOR_TIMER_H

#include "stock_registers.h"

static __sfr __at (0xc2) CCL1;
static __sfr __at (0xc3) CCH1;
static __sfr __at (0xd2) CML0;
static __sfr __at (0xd3) CMH0;
static __sfr __at (0xd4) CML1;
static __sfr __at (0xd5) CMH1;
static __sfr __at (0xd6) CML2;
static __sfr __at (0xd7) CMH2;
static __sfr __at (0xe2) CML3;
static __sfr __at (0xe3) CMH3;
static __sfr __at (0xf6) CMEN;

static __sbit __at (0xaf) EA;
static __sbit __at (0xe8) injector_output_0;
static __sbit __at (0xe9) injector_output_1;
static __sbit __at (0xea) injector_output_2;
static __sbit __at (0xeb) injector_output_3;

#define START_INJECTOR_PULSE(mask, capture, output, compare_low, compare_high) \
    do {                                                                       \
        CMEN &= (uint8_t)~(mask);                                              \
        EA = 0;                                                                \
        (output) = 0;                                                          \
        CCL1 = (capture);                                                      \
        EA = 1;                                                                \
        const uint16_t low_sum = (uint16_t)CCL1 + stock_r0;                    \
        stock_r2 = (uint8_t)low_sum;                                           \
        (compare_high) = CCH1 + stock_r1 + (low_sum >> 8);                     \
        (compare_low) = stock_r2;                                              \
        CMEN |= (mask);                                                        \
        (output) = 1;                                                          \
    } while (0)

#endif
