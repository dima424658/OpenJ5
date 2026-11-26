#include "stock_registers.h"

static __sfr __at (0xf0) B;
static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0xd7) CY;
static __sbit __at (0xf0) B_BIT0;
static __sbit __at (0xf1) B_BIT1;
static __sbit __at (0x1d) throttle_adc_low;
static __sbit __at (0x25) throttle_adc_high;
static __sbit __at (0x1e) maf_adc_low;
static __sbit __at (0x26) maf_adc_high;
static __sbit __at (0x46) full_barometric_correction;
static __sbit __at (0x49) tick_1s;

static volatile __data __at (4) uint8_t stock_r4;
static volatile __data __at (0x52) uint8_t throttle_position;
static volatile __data __at (0x57) uint8_t rpm_index_q4;
static volatile __data __at (0x59) uint8_t rpm_nearest_bin;
static volatile __xdata __at (0xf843) uint8_t air_charge_elapsed;
static volatile __xdata __at (0xf82a) uint8_t barometric_state_f82a;
static volatile __xdata __at (0xf896) uint8_t barometric_half_window;
static volatile __xdata __at (0xf897) uint8_t barometric_state_f897;

extern __code const uint8_t throttle_quantization_q4[101];
extern __code const uint8_t base_cylinder_charge_raw[256];
extern __code const int8_t barometric_correction_zone[256];
extern __code const uint8_t barometric_filter_length;

extern void interp_bilinear_u8(void);
extern void use_uncorrected_air_charge(void);
extern void learn_barometric_correction(void);
extern void select_partial_baro_weight(void);
extern void apply_reverse_flow_correction(void);

void check_barometric_neighbors(void);

void evaluate_air_charge_inputs(void)
{
    CY = throttle_adc_low | throttle_adc_high | maf_adc_low | maf_adc_high;
    if (CY)
        goto uncorrected;

    if (tick_1s) {
        ACC = air_charge_elapsed + 1;
        if (ACC != 0)
            air_charge_elapsed = ACC;
    }

    stock_r2 = rpm_index_q4;
    stock_r3 = throttle_quantization_q4[throttle_position];
    STOCK_DPTR = (uint16_t)base_cylinder_charge_raw;
    interp_bilinear_u8();
    stock_r3 = ACC;

    stock_r4 = throttle_quantization_q4[throttle_position];
    stock_r2 = ((uint8_t)(stock_r4 + 8) & 0xf0) + rpm_nearest_bin;
    ACC = (uint8_t)barometric_correction_zone[stock_r2];
    if (ACC != 0) {
        if (ACC_SIGN) {
            check_barometric_neighbors();
            return;
        }
        full_barometric_correction = 0;
        learn_barometric_correction();
        return;
    }

uncorrected:
    full_barometric_correction = 0;
    use_uncorrected_air_charge();
}

void check_barometric_neighbors(void)
{
    barometric_state_f82a = 0;
    barometric_half_window = barometric_filter_length >> 1;
    barometric_state_f897 = 0;

    B = 7;
    if (stock_r2 >= 16) {
        ACC = (uint8_t)barometric_correction_zone[stock_r2 - 16];
        if (!ACC_SIGN)
            B_BIT0 = 0;
    }
    if ((stock_r2 & 0x0f) < 15) {
        ACC = (uint8_t)barometric_correction_zone[stock_r2 + 1];
        if (!ACC_SIGN)
            B_BIT1 = 0;
    }

    if (stock_r2 >= 16 && (stock_r2 & 0x0f) < 15) {
        /* The stock interior branch checks the right-hand cell again. */
        ACC = (uint8_t)barometric_correction_zone[stock_r2 + 1];
        if (!ACC_SIGN)
            goto partial;
    } else if (B != 7) {
        goto partial;
    }

    full_barometric_correction = 1;
    stock_r4 = UINT8_MAX;
    apply_reverse_flow_correction();
    return;

partial:
    select_partial_baro_weight();
}
