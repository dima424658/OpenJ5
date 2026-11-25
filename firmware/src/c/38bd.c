#include "stock_registers.h"

static __sfr __at (0xf0) B;
static __sbit __at (0xf0) B_BIT0;
static __sbit __at (0xf1) B_BIT1;
static __sbit __at (0x46) full_barometric_correction;
static __sbit __at (0xaf) interrupts_enabled;
static volatile __data __at (0x00) uint8_t bank_zero_r0;
static volatile __data __at (0x01) uint8_t bank_zero_r1;
static volatile __data __at (0x04) uint8_t stock_r4;
static volatile __data __at (0x4e) uint8_t coolant_nearest_index;
static volatile __data __at (0x57) uint8_t rpm_index_q4;
static volatile __xdata __at (0xffc0) uint8_t retained_barometric_correction;
static volatile __xdata __at (0xf82c) uint16_t measured_air_charge;

extern __code const uint8_t reverse_flow_coolant_gain[39];
extern void use_air_charge_without_limit(void);
extern void publish_air_charge_limit(void);
extern void multiply_stock_u16_u8(void);
extern void shift_right_u16_2(void);
extern void add_u16_register_pairs(void);

void apply_reverse_flow_correction(void);

void select_partial_baro_weight(void)
{
    stock_r0 = (uint8_t)(stock_r4 - 8) & 0x0f;
    stock_r1 = (uint8_t)(7 - rpm_index_q4) & 0x0f;
    if (stock_r1 < stock_r0) {
        if (!B_BIT1)
            ACC = stock_r1;
        else
            ACC = stock_r0;
    } else if (B_BIT0) {
        ACC = stock_r1;
    } else {
        ACC = stock_r0;
    }

    full_barometric_correction = 0;
    stock_r4 = (uint8_t)(ACC << 4);
    if (stock_r4 == 0) {
        use_air_charge_without_limit();
        return;
    }
    apply_reverse_flow_correction();
}

void apply_reverse_flow_correction(void)
{
    stock_r01 = (uint16_t)stock_r3 * retained_barometric_correction;
    B = reverse_flow_coolant_gain[coolant_nearest_index];
    multiply_stock_u16_u8();
    shift_right_u16_2();
    if (stock_r4 != UINT8_MAX) {
        B = stock_r4;
        multiply_stock_u16_u8();

        /* The original copy reads physical bank-zero bytes. */
        stock_r2 = bank_zero_r0;
        stock_r3 = bank_zero_r1;
        interrupts_enabled = 0;
        stock_r01 = measured_air_charge;
        interrupts_enabled = 1;
        B = (uint8_t)-stock_r4;
        multiply_stock_u16_u8();
        add_u16_register_pairs();
    }
    publish_air_charge_limit();
}
