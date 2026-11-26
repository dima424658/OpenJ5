#include "stock_registers.h"

enum {
    HIGH_BIT = 0x80,
    MDU_SHIFT_RIGHT_ONE = 1,
    CYLINDER_MASK_OFFSET = 8,
};

#define SCORE_NUMERATOR UINT32_C(3472222222)

static __sfr __at (0xe9) mdu_0;
static __sfr __at (0xea) mdu_1;
static __sfr __at (0xeb) mdu_2;
static __sfr __at (0xec) mdu_3;
static __sfr __at (0xed) mdu_4;
static __sfr __at (0xee) mdu_5;
static __sfr __at (0xef) mdu_control;
static __sfr __at (0xf0) B;
static __sbit __at (0xd7) CY;
static __sbit __at (0xe4) ACC_SCORE_CROSSED;

static volatile __data __at (6) uint8_t stock_r6;
static volatile __data __at (7) uint8_t stock_r7;
static volatile __data __at (0x43) uint8_t knock_mask_index;
static volatile __xdata __at (0xf93d) uint8_t primary_misfire_conditions;
static volatile __xdata __at (0xf9e3) uint16_t misfire_score;
static volatile __xdata __at (0xf9e5) uint16_t cylinder_misfire_scores[4];
static volatile __xdata __at (0xf9ed) uint16_t corrected_period_delta;
static volatile __xdata __at (0xf9f1) uint16_t misfire_period_normalizer;
static volatile __xdata __at (0xf9f5) uint8_t misfire_score_flags;
static volatile __xdata __at (0xf9f9) uint8_t misfire_score_threshold;
static volatile __xdata __at (0xfa7e) uint8_t secondary_misfire_conditions;

extern volatile __code const uint8_t knock_event_masks[16];
extern void update_misfire_score_history(void);
void continue_misfire_period_checks(void);

#define WAIT_MDU_4() \
    __asm            \
        nop          \
        nop          \
        nop          \
        nop          \
    __endasm

#define WAIT_MDU_6() \
    __asm            \
        nop          \
        nop          \
        nop          \
        nop          \
        nop          \
        nop          \
    __endasm

#define LOAD_MDU_WORDS() \
    do {                 \
        mdu_0 = stock_r0; \
        mdu_1 = stock_r1; \
        mdu_2 = stock_r2; \
        mdu_3 = stock_r3; \
    } while (0)

#define READ_MDU_WORDS() \
    do {                 \
        stock_r0 = mdu_0; \
        stock_r1 = mdu_1; \
        stock_r2 = mdu_2; \
        stock_r3 = mdu_3; \
    } while (0)

#define DIVIDE_BY_REFERENCE_PERIOD() \
    do {                              \
        mdu_4 = stock_r6;             \
        mdu_5 = stock_r7;             \
        WAIT_MDU_6();                 \
    } while (0)

void calculate_misfire_period_score(void)
{
    mdu_0 = (uint8_t)SCORE_NUMERATOR;
    mdu_1 = (uint8_t)(SCORE_NUMERATOR >> 8);
    mdu_2 = (uint8_t)(SCORE_NUMERATOR >> 16);
    mdu_3 = (uint8_t)(SCORE_NUMERATOR >> 24);
    DIVIDE_BY_REFERENCE_PERIOD();

    // Discard the high byte of the first quotient, then scale by 256.
    stock_r0 = mdu_0;
    stock_r1 = mdu_1;
    stock_r2 = mdu_2;
    (void)mdu_3;
    (void)mdu_4;
    (void)mdu_5;
    mdu_0 = 0;
    mdu_1 = stock_r0;
    mdu_2 = stock_r1;
    mdu_3 = stock_r2;
    mdu_control = MDU_SHIFT_RIGHT_ONE;
    WAIT_MDU_6();

    READ_MDU_WORDS();
    LOAD_MDU_WORDS();
    DIVIDE_BY_REFERENCE_PERIOD();
    READ_MDU_WORDS();
    (void)mdu_4;
    (void)mdu_5;

    LOAD_MDU_WORDS();
    mdu_control = MDU_SHIFT_RIGHT_ONE;
    WAIT_MDU_6();
    READ_MDU_WORDS();

    LOAD_MDU_WORDS();
    DIVIDE_BY_REFERENCE_PERIOD();
    READ_MDU_WORDS();
    (void)mdu_4;
    (void)mdu_5;

    misfire_period_normalizer = stock_r01;
    stock_r23 = corrected_period_delta;
    mdu_0 = stock_r2;
    mdu_4 = stock_r0;
    mdu_1 = stock_r3;
    mdu_5 = stock_r1;
    WAIT_MDU_4();
    READ_MDU_WORDS();

    if (stock_r23 != 0) {
        stock_r01 = UINT16_MAX;
    }
    continue_misfire_period_checks();
}

void continue_misfire_period_checks(void)
{
    if ((primary_misfire_conditions & HIGH_BIT) && !(secondary_misfire_conditions & HIGH_BIT)) {
        stock_r01 = 0;
    }
    misfire_score = stock_r01;
    cylinder_misfire_scores[knock_mask_index] = stock_r01;

    B = misfire_score_threshold;
    if (stock_r1 < B) {
        CY = 0;
    } else {
        CY = 1;
    }
    ACC = misfire_score_flags;
    ACC_SCORE_CROSSED = CY;
    misfire_score_flags = ACC;

    if (CY) {
        B = knock_event_masks[CYLINDER_MASK_OFFSET + knock_mask_index];
        misfire_score_flags |= B;
    } else {
        B = ~knock_event_masks[CYLINDER_MASK_OFFSET + knock_mask_index];
        misfire_score_flags &= B;
    }
    update_misfire_score_history();
}
