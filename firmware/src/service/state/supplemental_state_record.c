#include "stock_registers.h"

static __sfr __at (0x86) sfr_86;
static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0x0e) result_bit_low;
static __sbit __at (0x0f) result_bit_high;
static __sbit __at (0x68) input_bit;
static __sbit __at (0xae) service_irq_enabled;
static __sbit __at (0xbe) service_irq_high_priority;
static __sbit __at (0xd7) CARRY;
static volatile __data __at (5) uint8_t stock_r5;
static volatile __data __at (6) uint8_t stock_r6;
static volatile __xdata __at (0xf4a0) uint8_t output_record[6];

extern void initialize_extra_state_scratch(void);
extern void absorb_extra_state_bit(void);
extern void generate_extra_state_pair(void);

#define RECORD_BYTE (*(volatile __idata uint8_t *)stock_r1)

#define SHIFT_ACC_LEFT_WITH_CARRY()            \
    do {                                       \
        const uint8_t low_bit = CARRY;         \
        CARRY = ACC_SIGN;                      \
        ACC = (uint8_t)((ACC << 1) | low_bit); \
    } while (0)

#define ABSORB_RECORD_BYTES(count)           \
    do {                                     \
        stock_r6 = (count);                  \
        do {                                 \
            stock_r5 = 8;                    \
            do {                             \
                ACC = RECORD_BYTE;           \
                SHIFT_ACC_LEFT_WITH_CARRY(); \
                RECORD_BYTE = ACC;           \
                input_bit = CARRY;           \
                absorb_extra_state_bit();    \
            } while (--stock_r5);            \
            ++stock_r1;                      \
        } while (--stock_r6);                \
    } while (0)

#define GENERATE_RECORD_BYTES(count)         \
    do {                                     \
        stock_r6 = (count);                  \
        do {                                 \
            stock_r5 = 4;                    \
            do {                             \
                generate_extra_state_pair(); \
                ACC = RECORD_BYTE;           \
                CARRY = result_bit_high;     \
                SHIFT_ACC_LEFT_WITH_CARRY(); \
                CARRY = result_bit_low;      \
                SHIFT_ACC_LEFT_WITH_CARRY(); \
                RECORD_BYTE = ACC;           \
            } while (--stock_r5);            \
            ++stock_r1;                      \
        } while (--stock_r6);                \
    } while (0)

void service_extra_state_b73f(void)
{
    sfr_86 = 0x80;
    service_irq_enabled = 1;
    service_irq_high_priority = 1;
    initialize_extra_state_scratch();

    ABSORB_RECORD_BYTES(4);
    stock_r1 = 0x25;
    GENERATE_RECORD_BYTES(4);
    stock_r1 = 0x29;
    ABSORB_RECORD_BYTES(2);
    stock_r1 = 0x29;
    GENERATE_RECORD_BYTES(2);

    input_bit = 0;
    stock_r2 = sizeof(output_record);
    stock_r0 = 0x25;
    STOCK_DPTR = (uint16_t)output_record;
    do {
        ACC = *(volatile __idata uint8_t *)stock_r0;
        *(volatile __xdata uint8_t *)STOCK_DPTR = ACC;
        ++stock_r0;
        ++STOCK_DPTR;
    } while (--stock_r2);
}
