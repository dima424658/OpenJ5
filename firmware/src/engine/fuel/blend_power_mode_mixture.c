#include "stock_registers.h"

static __sfr __at (0xe9) mdu_0;
static __sfr __at (0xea) mdu_1;
static __sfr __at (0xed) mdu_4;
static __sfr __at (0xee) mdu_5;
static __sfr __at (0xf0) B;
static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0x44) mdu_interrupted;
static __sbit __at (0x5a) at_or_above_power_boundary;

static volatile __data __at (0x4e) uint8_t coolant_index;
static volatile __data __at (0x52) uint8_t throttle_position;
static volatile __data __at (0x57) uint8_t rpm_q4;
static volatile __data __at (0x58) uint8_t rpm_half_bin;
static volatile __data __at (0x5c) uint8_t charge_q4;
static volatile __data __at (0x5f) uint8_t operating_point_index;
static volatile __data __at (0x60) uint8_t requested_mixture;
static volatile __data __at (0x62) uint8_t requested_ignition;
static volatile __data __at (0x7a) uint8_t ignition_and_airflow_control_7a;

static volatile __xdata __at (0xf876) uint8_t load_warmup_correction_raw;
static volatile __xdata __at (0xf87b) uint8_t power_mode_weight;
static volatile __xdata __at (0xf87f) uint8_t warmup_flags;

extern __code const uint8_t economy_throttle_boundary[32];
extern __code const uint8_t power_transition_width[32];
extern __code const uint8_t power_ignition_raw[256];
extern __code const uint8_t power_mixture_raw[256];
extern __code const uint8_t coolant_ignition_correction_raw[39];
extern __code const uint8_t cold_power_ignition_corr_raw[256];

extern void interp_bilinear_u8(void);
extern void blend_u8_q8(void);

#define SAMPLE_POWER_MAP(table)                                              \
    do {                                                                     \
        STOCK_DPTR = (uint16_t)(table);                                      \
        stock_r2 = rpm_q4;                                                   \
        stock_r3 = charge_q4;                                                \
        interp_bilinear_u8();                                                \
    } while (0)

#define BLEND_POWER_TARGET(table, target)                                    \
    do {                                                                     \
        SAMPLE_POWER_MAP(table);                                             \
        stock_r2 = ACC;                                                      \
        stock_r0 = (target);                                                 \
        stock_r3 = power_mode_weight;                                        \
        blend_u8_q8();                                                       \
        (target) = ACC;                                                      \
    } while (0)

#define CORRECT_POWER_IGNITION()                                             \
    do {                                                                     \
        B = coolant_ignition_correction_raw[coolant_index];                  \
        B += load_warmup_correction_raw;                                     \
        B = requested_ignition - B;                                          \
        if (warmup_flags & 2) {                                              \
            B += cold_power_ignition_corr_raw[operating_point_index];  \
        }                                                                    \
        requested_ignition = B;                                              \
    } while (0)

void correct_transition_ignition(void);
void select_full_power_mixture(void);

void blend_power_mode_mixture(void)
{
    B = power_transition_width[rpm_half_bin];
    stock_r1 = economy_throttle_boundary[rpm_half_bin];
    const uint8_t throttle = throttle_position;
    at_or_above_power_boundary = throttle >= stock_r1;
    if (throttle <= stock_r1) {
        correct_transition_ignition();
        return;
    }

    stock_r1 = throttle - stock_r1;
    if (stock_r1 >= B) {
        select_full_power_mixture();
        return;
    }

    /* Compute the Q8 weight `(256 * throttle excess) / width`. */
    do {
        mdu_interrupted = 0;
        mdu_0 = 0;
        mdu_1 = stock_r1;
        mdu_4 = B;
        mdu_5 = 0;
        __asm
            nop
            nop
            nop
            nop
        __endasm;
        stock_r3 = mdu_0;
        ACC = mdu_1;
        ACC = mdu_4;
        ACC = mdu_5;
    } while (mdu_interrupted);
    power_mode_weight = stock_r3;

    if (!(ignition_and_airflow_control_7a & 4)) {
        BLEND_POWER_TARGET(power_ignition_raw, requested_ignition);
    }
    BLEND_POWER_TARGET(power_mixture_raw, requested_mixture);
    correct_transition_ignition();
}

void correct_transition_ignition(void)
{
    if (!(ignition_and_airflow_control_7a & 4)) {
        CORRECT_POWER_IGNITION();
    }
}

void select_full_power_mixture(void)
{
    if (!(ignition_and_airflow_control_7a & 4)) {
        SAMPLE_POWER_MAP(power_ignition_raw);
        requested_ignition = ACC;
        CORRECT_POWER_IGNITION();
    }
    SAMPLE_POWER_MAP(power_mixture_raw);
    requested_mixture = ACC;
}
