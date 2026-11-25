#include "stock_registers.h"

enum { FUEL_FILM_BASE = 0x39 };

#define FUEL_FILM_OFFSET \
    ((uint8_t)(FUEL_FILM_BASE + (uint8_t)((crank_slot << 1) | (crank_slot >> 7))))
#define FUEL_FILM_BYTE(offset) \
    (*(volatile __xdata uint8_t *)(UINT16_C(0xf800) | (uint8_t)(offset)))

static __sfr __at (0xc2) sfr_c2;
static __sfr __at (0xc3) sfr_c3;
static __sfr __at (0xe6) sfr_e6;
static __sfr __at (0xe7) sfr_e7;
static __sfr __at (0xe9) mdu_0;
static __sfr __at (0xea) mdu_1;
static __sfr __at (0xeb) mdu_2;
static __sfr __at (0xec) mdu_3;
static __sfr __at (0xed) mdu_4;
static __sfr __at (0xee) mdu_5;
static __sfr __at (0xf0) B;
static __sfr __at (0xf6) sfr_f6;

static __sbit __at (0x05) fuel_configuration_flag_05;
static __sbit __at (0x14) injection_selection_flag_14;
static __sbit __at (0x45) output_state_flag_45;
static __sbit __at (0x50) split_injection_dose;
static __sbit __at (0x5b) deceleration_fuel_cut;
static __sbit __at (0xaf) EA;
static __sbit __at (0xd7) CY;
static __sbit __at (0xed) timer_latch_ed;

static volatile __data __at (4) uint8_t stock_r4;
static volatile __data __at (0x08) uint16_t bank_one_r01;
static volatile __data __at (0x0a) uint16_t bank_one_r23;
static volatile __data __at (0x41) uint8_t crank_slot;
static volatile __data __at (0x4e) uint8_t coolant_nearest_index;
static volatile __data __at (0x54) uint8_t rpm_axis;
static volatile __data __at (0x5d) uint8_t charge_nearest_half_bin;
static volatile __data __at (0x61) uint8_t requested_mixture_code;
static volatile __xdata __at (0xf836) uint8_t fuel_film_gain;
static volatile __xdata __at (0xf837) uint16_t base_cyclic_fuel;
static volatile __xdata __at (0xf89b) uint16_t poststart_fuel_floor;
static volatile __xdata __at (0xf89d) uint8_t fuel_film_events;
static volatile __xdata __at (0xf8b1) uint8_t fuel_transition_event_count;
static volatile __xdata __at (0xf939) uint16_t cyclic_fuel_accumulator;
static volatile __xdata __at (0xf97b) uint16_t opening_fuel_delta;
static volatile __xdata __at (0xf97f) uint8_t fuel_weight;
static volatile __xdata __at (0xf987) uint16_t delivered_cyclic_fuel;

extern volatile __code const uint8_t injection_cutoff_rpm_code;
extern __code const uint8_t fuel_film_charge_gain[32];
extern __code const uint8_t fuel_film_coolant_gain[39];
extern __code const uint8_t fuel_film_resume_gain[32];

extern void add_u16_register_pairs(void);
extern void subtract_u16_register_pairs(void);
extern void multiply_stock_u16_u8(void);
extern void shift_left_saturating_1(void);
extern void shift_left_saturating_3(void);
extern void clamp_stock_u8(void);
extern void apply_fuel_corrections(void);
extern void publish_cyclic_fuel(void);
extern void apply_minimum_fuel_dose(void);

void track_cyclic_fuel(void);
void calculate_fuel_film(void);
void update_cylinder_fuel_film(void);
void update_film_after_underflow(void);
void add_throttle_fuel_delta(void);

void calculate_running_fuel(void)
{
    if (!deceleration_fuel_cut) {
        B = injection_cutoff_rpm_code;
        if (rpm_axis < B) {
            const uint16_t mixture = 128u + requested_mixture_code;
            B = (uint8_t)mixture;
            ACC = mixture >> 8;
            mdu_0 = 0;
            mdu_1 = stock_r0;
            mdu_2 = stock_r1;
            mdu_3 = 0;
            mdu_4 = B;
            mdu_5 = ACC;
            __asm
                nop
                nop
                nop
                nop
                nop
                nop
            __endasm;
            stock_r0 = mdu_0;
            stock_r1 = mdu_1;
            (void)mdu_2;
            (void)mdu_3;
            (void)mdu_4;
            (void)mdu_5;

            B = fuel_weight;
            multiply_stock_u16_u8();
            shift_left_saturating_1();
            base_cyclic_fuel = stock_r01;
            stock_r23 = poststart_fuel_floor;
            subtract_u16_register_pairs();
            if (CY) {
                base_cyclic_fuel = stock_r23;
            }
            track_cyclic_fuel();
            return;
        }
    }
    stock_r01 = 0;
    delivered_cyclic_fuel = stock_r01;
    publish_cyclic_fuel();
}

void track_cyclic_fuel(void)
{
    stock_r01 = cyclic_fuel_accumulator;
    stock_r23 = delivered_cyclic_fuel;
    add_u16_register_pairs();
    cyclic_fuel_accumulator = stock_r01;
    stock_r23 = split_injection_dose ? 0x0682 : 0x0d04;
    subtract_u16_register_pairs();
    if (!CY) {
        cyclic_fuel_accumulator = stock_r01;
        ACC = stock_r1;
        sfr_f6 &= 0xdf;
        EA = 0;
        timer_latch_ed = 1;
        sfr_c2 = ACC;
        EA = 1;
        const uint16_t timer_value = (uint16_t)sfr_c2 + 0x94u;
        stock_r2 = (uint8_t)timer_value;
        sfr_e7 = sfr_c3 + (timer_value >> 8);
        sfr_e6 = stock_r2;
        sfr_f6 |= 0x20;
        timer_latch_ed = 0;
    }
    calculate_fuel_film();
}

void calculate_fuel_film(void)
{
    const uint8_t gain = ((uint16_t)fuel_film_charge_gain[charge_nearest_half_bin] *
        fuel_film_coolant_gain[coolant_nearest_index]) >> 8;
    if (fuel_film_events < UINT8_MAX) {
        ++fuel_film_events;
    }
    stock_r1 = fuel_film_events;
    stock_r0 = 0;
    ACC = gain;
    clamp_stock_u8();
    fuel_film_gain = ACC;
    stock_r4 = ACC;

    stock_r01 = base_cyclic_fuel;
    B = stock_r4;
    multiply_stock_u16_u8();
    shift_left_saturating_3();
    if (output_state_flag_45) {
        const uint8_t offset = FUEL_FILM_OFFSET;
        FUEL_FILM_BYTE(offset) = stock_r0;
        FUEL_FILM_BYTE(offset + 1) = stock_r1;
        stock_r01 = base_cyclic_fuel;
        add_throttle_fuel_delta();
        return;
    }
    update_cylinder_fuel_film();
}

void update_cylinder_fuel_film(void)
{
    bank_one_r23 = bank_one_r01;
    stock_r01 = base_cyclic_fuel;
    add_u16_register_pairs();

    const uint8_t offset = FUEL_FILM_OFFSET;
    const uint8_t old_low = FUEL_FILM_BYTE(offset);
    FUEL_FILM_BYTE(offset) = stock_r2;
    stock_r2 = old_low;
    const uint8_t old_high = FUEL_FILM_BYTE(offset + 1);
    FUEL_FILM_BYTE(offset + 1) = stock_r3;
    stock_r3 = old_high;
    subtract_u16_register_pairs();
    ACC = stock_r1;
    if (ACC_SIGN) {
        update_film_after_underflow();
        return;
    }
    if (fuel_transition_event_count != 0) {
        --fuel_transition_event_count;
        B = fuel_film_resume_gain[charge_nearest_half_bin];
        bank_one_r23 = bank_one_r01;
        multiply_stock_u16_u8();
        add_u16_register_pairs();
    }
    add_throttle_fuel_delta();
}

void update_film_after_underflow(void)
{
    B = stock_r4;
    ACC += 0x20;
    mdu_0 = 0;
    mdu_1 = B;
    mdu_4 = ACC;
    mdu_5 = 0;
    __asm
        nop
        nop
        nop
        nop
    __endasm;
    B = mdu_0;
    (void)mdu_1;
    (void)mdu_4;
    (void)mdu_5;
    stock_r01 = bank_one_r23;
    multiply_stock_u16_u8();

    const uint8_t offset = FUEL_FILM_OFFSET;
    FUEL_FILM_BYTE(offset) = stock_r0;
    FUEL_FILM_BYTE(offset + 1) = stock_r1;
    stock_r01 = 0;
    add_throttle_fuel_delta();
}

void add_throttle_fuel_delta(void)
{
    stock_r23 = opening_fuel_delta;
    add_u16_register_pairs();
    if (stock_r01 == 0) {
        apply_minimum_fuel_dose();
        return;
    }
    if (fuel_configuration_flag_05) {
        split_injection_dose = injection_selection_flag_14;
    } else {
        split_injection_dose = 1;
    }
    apply_fuel_corrections();
}
