#include "stock_registers.h"

static __sbit __at (0xd7) CARRY;
static __sbit __at (0xe0) ACC_BIT0;
static __sbit __at (0x00) state_bit_00;
static __sbit __at (0x01) state_bit_01;
static __sbit __at (0x08) state_bit_08;
static __sbit __at (0x09) state_bit_09;
static __sbit __at (0x0a) state_bit_0a;
static __sbit __at (0x0b) state_bit_0b;
static __sbit __at (0x0c) state_bit_0c;
static __sbit __at (0x0d) state_bit_0d;
static __sbit __at (0x0e) state_bit_0e;
static __sbit __at (0x0f) state_bit_0f;
static __sbit __at (0x10) state_bit_10;
static __sbit __at (0x11) state_bit_11;
static __sbit __at (0x12) state_bit_12;
static __sbit __at (0x13) state_bit_13;
static __sbit __at (0x14) state_bit_14;
static __sbit __at (0x15) state_bit_15;
static __sbit __at (0x16) state_bit_16;
static __sbit __at (0x17) state_bit_17;
static __sbit __at (0x18) state_bit_18;
static __sbit __at (0x19) state_bit_19;
static __sbit __at (0x1a) state_bit_1a;
static __sbit __at (0x1b) state_bit_1b;
static __sbit __at (0x1c) state_bit_1c;
static __sbit __at (0x1d) state_bit_1d;
static __sbit __at (0x1e) state_bit_1e;
static __sbit __at (0x1f) state_bit_1f;
static __sbit __at (0x21) state_bit_21;
static __sbit __at (0x22) state_bit_22;
static __sbit __at (0x24) state_bit_24;
static __sbit __at (0x26) state_bit_26;
static __sbit __at (0x68) input_bit;

static volatile __data __at (4) uint8_t stock_r4;
static volatile __data __at (0x20) uint8_t state_mask;

extern const __code uint8_t supplemental_state_lookup[256];
extern void mix_next_extra_state_byte(void);
extern void mix_extra_state_byte(void);

#define LOOKUP_BYTE (supplemental_state_lookup[ACC])
#define APPEND_STATE_BIT(bit) (ACC = (uint8_t)((ACC << 1) | (bit)))

#define COLLECT_FIRST_MASK_BITS(mode_bit) \
    do {                                  \
        ACC = 0;                          \
        APPEND_STATE_BIT(state_bit_16);   \
        APPEND_STATE_BIT(state_bit_0a);   \
        APPEND_STATE_BIT(state_bit_1b);   \
        APPEND_STATE_BIT(mode_bit);       \
        APPEND_STATE_BIT(state_bit_18);   \
        APPEND_STATE_BIT(state_bit_1c);   \
        APPEND_STATE_BIT(state_bit_0e);   \
        ACC = LOOKUP_BYTE & 0xa0;         \
        state_mask = ACC;                 \
    } while (0)

#define COLLECT_SECOND_MASK_BITS()                     \
    do {                                               \
        ACC = (uint8_t)((ACC << 1) | (ACC >> 7));      \
        APPEND_STATE_BIT(state_bit_1d);                \
        APPEND_STATE_BIT(state_bit_0c);                \
        APPEND_STATE_BIT(state_bit_12);                \
        APPEND_STATE_BIT(state_bit_19);                \
        APPEND_STATE_BIT(state_bit_13);                \
        APPEND_STATE_BIT(state_bit_1e);                \
        APPEND_STATE_BIT(state_bit_0f);                \
        ACC = LOOKUP_BYTE & 0x09;                      \
        state_mask |= ACC;                             \
        const uint8_t high_bit = CARRY;                \
        CARRY = ACC_BIT0;                              \
        ACC = (uint8_t)((ACC >> 1) | (high_bit << 7)); \
    } while (0)

#define COLLECT_FINAL_MASK_BITS()              \
    do {                                       \
        state_bit_00 = CARRY;                  \
        const uint8_t low_bit = CARRY;         \
        CARRY = ACC_SIGN;                      \
        ACC = (uint8_t)((ACC << 1) | low_bit); \
        APPEND_STATE_BIT(state_bit_1f);        \
        APPEND_STATE_BIT(state_bit_10);        \
        APPEND_STATE_BIT(state_bit_17);        \
        APPEND_STATE_BIT(state_bit_24);        \
        APPEND_STATE_BIT(state_bit_09);        \
        APPEND_STATE_BIT(state_bit_26);        \
        APPEND_STATE_BIT(state_bit_0d);        \
        ACC = LOOKUP_BYTE & 0x12;              \
        state_mask |= ACC;                     \
        APPEND_STATE_BIT(state_bit_01);        \
        APPEND_STATE_BIT(state_bit_1a);        \
        APPEND_STATE_BIT(state_bit_0b);        \
        APPEND_STATE_BIT(state_bit_08);        \
        APPEND_STATE_BIT(state_bit_14);        \
        APPEND_STATE_BIT(state_bit_22);        \
        APPEND_STATE_BIT(state_bit_21);        \
        APPEND_STATE_BIT(state_bit_15);        \
        ACC = LOOKUP_BYTE & 0x44;              \
        state_mask |= ACC;                     \
    } while (0)

void absorb_extra_state_bit(void)
{
    stock_r4 = 17;
    do {
        COLLECT_FIRST_MASK_BITS(input_bit);
        COLLECT_SECOND_MASK_BITS();
        if (input_bit) {
            CARRY = !CARRY;
        }
        COLLECT_FINAL_MASK_BITS();
        mix_next_extra_state_byte();
    } while (--stock_r4);
}

void generate_extra_state_pair(void)
{
    stock_r4 = 34;
    do {
        COLLECT_FIRST_MASK_BITS(state_bit_11);
        COLLECT_SECOND_MASK_BITS();
        CARRY = !CARRY;
        COLLECT_FINAL_MASK_BITS();

        const uint8_t high_bit = CARRY;
        ACC = stock_r4;
        CARRY = ACC_BIT0;
        ACC = (uint8_t)((ACC >> 1) | (high_bit << 7));
        if (CARRY) {
            mix_next_extra_state_byte();
        } else {
            mix_extra_state_byte();
        }
    } while (--stock_r4);
}
