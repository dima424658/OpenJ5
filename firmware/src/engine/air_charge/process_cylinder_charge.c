#include "stock_registers.h"

static __sfr16 __at (0x8382) STOCK_DPTR;
static __sfr __at (0xf0) B;
static __sbit __at (0xd7) CY;
static __sbit __at (0x45) output_state_flag_45;
static __sbit __at (0x53) startup_flag_53;

static volatile __data __at (0x41) uint8_t crank_state_41;
static volatile __data __at (0x4e) uint8_t coolant_nearest_index;
static volatile __data __at (0x57) uint8_t rpm_q4;
static volatile __xdata __at (0xf82e) uint16_t cylinder_charge;
static volatile __xdata __at (0xf87e) uint8_t charge_opening_increment;
static volatile __xdata __at (0xf8ab) uint8_t previous_mapped_charge;
static volatile __xdata __at (0xf8b8) uint8_t previous_throttle_position;
static volatile __xdata __at (0xf97b) uint16_t opening_fuel_delta;
static volatile __xdata __at (0xf97d) uint16_t opening_fuel_candidate;
static volatile __xdata __at (0xf98c) uint8_t crank_throttle_position;
static volatile __xdata __at (0xf98d) uint16_t closing_fuel_delta;
static volatile __xdata __at (0xf98f) uint16_t closing_fuel_candidate;

extern volatile __code const uint8_t opening_fuel_decay_1;
extern volatile __code const uint8_t opening_fuel_decay_2;
extern volatile __code const uint8_t closing_charge_gain;
extern volatile __code const uint8_t closing_throttle_deadband;
extern volatile __code const uint8_t closing_fuel_decay_1;
extern volatile __code const uint8_t closing_fuel_decay_2;
extern volatile __code const uint8_t opening_throttle_deadband;
extern __code const uint8_t opening_charge_gain[39];
extern __code const uint8_t base_cylinder_charge_raw[16][16];
extern __code const uint8_t throttle_quantization_q4[101];

extern void interp_bilinear_u8(void);
extern void multiply_stock_u16_u8(void);
extern void subtract_u16_register_pairs(void);
extern void shift_right_u16_2(void);
extern void finish_throttle_charge_update(void);

void update_charge_on_closing(void);
void store_closing_charge_delta(void);
void update_charge_on_opening(void);
void store_opening_charge_delta(void);
void write_opening_charge_delta(void);
void decay_closing_charge_delta(void);
void reset_throttle_charge_deltas(void);

void process_cylinder_charge(void)
{
    cylinder_charge = stock_r01;
    if (output_state_flag_45 || startup_flag_53) {
        reset_throttle_charge_deltas();
        return;
    }

    B = crank_throttle_position;
    stock_r3 = throttle_quantization_q4[B];
    STOCK_DPTR = (uint16_t)base_cylinder_charge_raw;
    stock_r2 = rpm_q4;
    interp_bilinear_u8();
    stock_r2 = ACC;

    B = previous_throttle_position;
    const uint8_t throttle = crank_throttle_position;
    ACC = throttle - B;
    if (throttle < B) {
        update_charge_on_closing();
        return;
    }
    update_charge_on_opening();
}

void update_charge_on_closing(void)
{
    if (!output_state_flag_45) {
        B = -ACC;
        if (B > closing_throttle_deadband) {
            B = previous_mapped_charge;
            previous_mapped_charge = stock_r2;
            if (B > stock_r2) {
                B -= stock_r2;
                stock_r01 = (uint16_t)closing_charge_gain * B;
                shift_right_u16_2();
                store_closing_charge_delta();
                return;
            }
        } else {
            previous_mapped_charge = stock_r2;
        }
    }

    stock_r01 = closing_fuel_delta;
    B = closing_fuel_decay_1;
    multiply_stock_u16_u8();
    store_closing_charge_delta();
}

void store_closing_charge_delta(void)
{
    closing_fuel_candidate = stock_r01;
    stock_r23 = closing_fuel_delta;
    subtract_u16_register_pairs();
    if (CY) {
        stock_r01 = closing_fuel_delta;
        B = closing_fuel_decay_2;
        multiply_stock_u16_u8();
    } else {
        stock_r01 = closing_fuel_candidate;
    }
    closing_fuel_delta = stock_r01;

    stock_r01 = opening_fuel_delta;
    B = opening_fuel_decay_1;
    multiply_stock_u16_u8();
    opening_fuel_delta = stock_r01;
    opening_fuel_candidate = stock_r01;
    finish_throttle_charge_update();
}

void update_charge_on_opening(void)
{
    if (!output_state_flag_45) {
        B = ACC;
        if (B > opening_throttle_deadband) {
            B = previous_mapped_charge;
            previous_mapped_charge = stock_r2;
            if (stock_r2 > B) {
                B = stock_r2 - B;
                charge_opening_increment = B;
                stock_r01 = (uint16_t)opening_charge_gain[coolant_nearest_index] * B;
                shift_right_u16_2();
                store_opening_charge_delta();
                return;
            }
        } else {
            previous_mapped_charge = stock_r2;
        }
    }

    if (crank_state_41 == 0) {
        stock_r01 = opening_fuel_delta;
        B = opening_fuel_decay_1;
        multiply_stock_u16_u8();
        write_opening_charge_delta();
        return;
    }
    decay_closing_charge_delta();
}

void store_opening_charge_delta(void)
{
    opening_fuel_candidate = stock_r01;
    stock_r23 = opening_fuel_delta;
    subtract_u16_register_pairs();
    if (CY) {
        stock_r01 = opening_fuel_delta;
        B = opening_fuel_decay_2;
        multiply_stock_u16_u8();
    } else {
        stock_r01 = opening_fuel_candidate;
    }
    write_opening_charge_delta();
}

void write_opening_charge_delta(void)
{
    opening_fuel_delta = stock_r01;
    decay_closing_charge_delta();
}

void decay_closing_charge_delta(void)
{
    stock_r01 = closing_fuel_delta;
    B = closing_fuel_decay_2;
    multiply_stock_u16_u8();
    closing_fuel_delta = stock_r01;
    closing_fuel_candidate = stock_r01;
    finish_throttle_charge_update();
}

void reset_throttle_charge_deltas(void)
{
    opening_fuel_delta = 0;
    opening_fuel_candidate = 0;
    closing_fuel_delta = 0;
    closing_fuel_candidate = 0;
    finish_throttle_charge_update();
}
