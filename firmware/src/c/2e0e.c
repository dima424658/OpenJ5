#include "stock_registers.h"

static __sfr __at (0xf0) B;
static __sbit __at (0xd7) CY;
static __sfr __at (0xe9) mdu_0;
static __sfr __at (0xea) mdu_1;
static __sfr __at (0xeb) mdu_2;
static __sfr __at (0xec) mdu_3;
static __sfr __at (0xed) mdu_4;
static __sfr __at (0xee) mdu_5;

static volatile __data __at (0x55) uint8_t rpm_eighth;
static volatile __xdata __at (0xf93f) uint8_t misfire_rpm_position;
/* The last table entry also reads the word immediately after the four active words. */
static volatile __xdata __at (0xfa53) uint8_t misfire_word_bytes[10];

extern __code const uint8_t misfire_rpm_positions[128];
extern void subtract_u16_register_pairs(void);
extern void add_u16_register_pairs(void);
extern void negate_u16_register_pair(void);
extern void apply_misfire_word_bounds(void);

void scale_misfire_falling_pair(void);
void scale_misfire_rising_pair(void);

void continue_misfire_rpm(void)
{
    ACC = rpm_eighth;
    if (ACC >= 128)
        ACC = 127;
    ACC = misfire_rpm_positions[ACC];
    misfire_rpm_position = ACC;

    B = ACC >> 4;
    const volatile __xdata uint8_t *pair = &misfire_word_bytes[B];
    stock_r2 = pair[0];
    stock_r3 = pair[1];
    stock_r0 = pair[2];
    stock_r1 = pair[3];
    subtract_u16_register_pairs();
    if (CY) {
        scale_misfire_falling_pair();
        return;
    }
    scale_misfire_rising_pair();
}

inline void multiply_misfire_fraction(void)
{
    /* Multiply the magnitude by the Q4 fraction, scaled to the high nibble. */
    mdu_0 = (uint8_t)(misfire_rpm_position << 4);
    mdu_4 = stock_r0;
    mdu_1 = 0;
    mdu_5 = stock_r1;
    __asm
        nop
        nop
        nop
        nop
    __endasm;

    ACC = mdu_0;
    stock_r0 = mdu_1;
    stock_r1 = mdu_2;
    B = mdu_3;
    stock_r01 += ACC >> 7;
}

void scale_misfire_rising_pair(void)
{
    multiply_misfire_fraction();
    add_u16_register_pairs();
    apply_misfire_word_bounds();
}

void scale_misfire_falling_pair(void)
{
    negate_u16_register_pair();
    multiply_misfire_fraction();
    subtract_u16_register_pairs();
    negate_u16_register_pair();
    apply_misfire_word_bounds();
}
