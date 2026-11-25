#include "stock_registers.h"

enum { BAROMETRIC_ZONE_COUNT = 32, BAROMETRIC_PUBLISH_SECONDS = 10 };

static __sfr __at (0xf0) B;
static __sbit __at (0x44) mdu_interrupted;
static __sbit __at (0xaf) interrupts_enabled;
static __sfr __at (0xe9) mdu_0;
static __sfr __at (0xea) mdu_1;
static __sfr __at (0xeb) mdu_2;
static __sfr __at (0xec) mdu_3;
static __sfr __at (0xed) mdu_4;
static __sfr __at (0xee) mdu_5;
static volatile __data __at (0x04) uint8_t stock_r4;

static volatile __xdata __at (0xf82a) uint8_t barometric_sample_count;
static volatile __xdata __at (0xf830) uint16_t air_charge_limit;
static volatile __xdata __at (0xf843) uint8_t air_charge_elapsed;
static volatile __xdata __at (0xf896) uint8_t barometric_sum_low;
static volatile __xdata __at (0xf897) uint8_t barometric_sum_high;
static volatile __xdata __at (0xf8d0) uint8_t barometric_history[BAROMETRIC_ZONE_COUNT];
static volatile __xdata __at (0xffc0) uint8_t retained_barometric_correction;

extern __code const uint8_t barometric_filter_length;
extern volatile __code const uint8_t barometric_correction_min_q7;
extern volatile __code const uint8_t barometric_correction_max_q7;

extern void shift_left_saturating_3(void);
extern void clamp_stock_u8(void);
extern void finish_sensor_diagnostics(void);

void publish_barometric_history(void);
void use_uncorrected_air_charge(void);
void use_air_charge_without_limit(void);
void publish_air_charge_limit(void);

void average_baro_zone_samples(void)
{
    do {
        mdu_interrupted = 0;
        mdu_0 = stock_r0;
        mdu_1 = stock_r1;
        mdu_4 = B;
        mdu_5 = 0;
        __asm
            nop
            nop
            nop
            nop
        __endasm;

        stock_r2 = mdu_0;
        ACC = mdu_1;
        ACC = mdu_2;
        ACC = mdu_3;
    } while (mdu_interrupted);
    barometric_history[(uint8_t)(stock_r4 - 1)] = stock_r2;

    if (air_charge_elapsed < BAROMETRIC_PUBLISH_SECONDS) {
        use_uncorrected_air_charge();
        return;
    }
    publish_barometric_history();
}

void publish_barometric_history(void)
{
    air_charge_elapsed = 0;
    stock_r01 = BAROMETRIC_ZONE_COUNT / 2;
    B = BAROMETRIC_ZONE_COUNT;
    volatile __xdata uint8_t *entry = barometric_history;
    do {
        stock_r01 += *entry++;
    } while (--B != 0);
    shift_left_saturating_3();

    stock_r0 = barometric_correction_min_q7;
    ACC = barometric_correction_max_q7;
    const uint8_t estimate = stock_r1;
    stock_r1 = ACC;
    ACC = estimate;
    clamp_stock_u8();
    retained_barometric_correction = ACC;

    B = BAROMETRIC_ZONE_COUNT;
    entry = barometric_history;
    do {
        *entry++ = ACC;
    } while (--B != 0);
    use_uncorrected_air_charge();
}

void use_uncorrected_air_charge(void)
{
    barometric_sample_count = 0;
    barometric_sum_low = barometric_filter_length >> 1;
    barometric_sum_high = 0;
    use_air_charge_without_limit();
}

void use_air_charge_without_limit(void)
{
    stock_r01 = UINT16_MAX;
    publish_air_charge_limit();
}

void publish_air_charge_limit(void)
{
    interrupts_enabled = 0;
    air_charge_limit = stock_r01;
    interrupts_enabled = 1;
    finish_sensor_diagnostics();
}
