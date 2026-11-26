#include "stock_registers.h"

enum { CYLINDER_MASK_OFFSET = 8 };

static __sfr __at (0xf0) B;
static __sbit __at (0xd7) CY;
static __sbit __at (0xe4) ACC_SCORE_CROSSED;
static __sbit __at (0x59) idle_mode;

static volatile __data __at (0x0a) uint16_t bank_one_r23;
static volatile __data __at (0x43) uint8_t knock_mask_index;
static volatile __xdata __at (0xf9e3) uint16_t misfire_score;
static volatile __xdata __at (0xf9fb) uint8_t misfire_filtered_threshold;
static volatile __xdata __at (0xfa00) uint16_t filtered_misfire_scores[4];
static volatile __xdata __at (0xfa09) uint8_t misfire_filtered_flags;

extern volatile __code const uint8_t misfire_score_weight;
extern volatile __code const uint8_t misfire_score_idle_weight;
extern volatile __code const uint8_t knock_event_masks[16];

extern void multiply_stock_u16_u8(void);
extern void add_u16_register_pairs(void);
extern void update_misfire_period_terms(void);

void update_misfire_score_history(void)
{
    stock_r01 = filtered_misfire_scores[knock_mask_index];
    B = -(idle_mode ? misfire_score_idle_weight : misfire_score_weight);
    multiply_stock_u16_u8();

    // In the interrupt's bank 1, the fixed destination is the R3:R2 addend.
    bank_one_r23 = stock_r01;

    stock_r01 = misfire_score;
    B = idle_mode ? misfire_score_idle_weight : misfire_score_weight;
    multiply_stock_u16_u8();
    add_u16_register_pairs();
    filtered_misfire_scores[knock_mask_index] = stock_r01;

    B = misfire_filtered_threshold;
    if (stock_r1 < B) {
        CY = 0;
    } else {
        CY = 1;
    }
    ACC = misfire_filtered_flags;
    ACC_SCORE_CROSSED = CY;
    misfire_filtered_flags = ACC;

    if (CY) {
        B = knock_event_masks[CYLINDER_MASK_OFFSET + knock_mask_index];
        misfire_filtered_flags |= B;
    } else {
        B = ~knock_event_masks[CYLINDER_MASK_OFFSET + knock_mask_index];
        misfire_filtered_flags &= B;
    }
    update_misfire_period_terms();
}
