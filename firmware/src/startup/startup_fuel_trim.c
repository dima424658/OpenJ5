#include "retained_ram.h"

static __sfr __at (0xf0) B;
static __sbit __at (0xe0) ACC_BIT0;
static __sbit __at (0xe1) ACC_BIT1;
static __sbit __at (0x15) startup_mode_15;
static __sbit __at (0x16) retained_ram_checksum_failed;
static volatile __xdata __at (0xf8d0) uint8_t barometric_history[32];
static volatile __xdata __at (0xf9b0) uint8_t retained_status;
static volatile __xdata __at (0xf518) uint8_t retained_field_f518;
static volatile __xdata __at (0xffc0) uint8_t retained_barometric_correction;

extern volatile __code const uint8_t barometric_correction_min_q7;
extern volatile __code const uint8_t barometric_correction_max_q7;
extern volatile __code const uint8_t retain_startup_data_config;

extern void clamp_stock_u8(void);
extern void clear_retained_words_ce94(void);
extern void filter_retained_entries(void);
extern void sample_startup_coolant(void);

void init_startup_baro_history(void);

void continue_startup_fuel_trim(void)
{
    stock_r0 = barometric_correction_min_q7;
    stock_r1 = barometric_correction_max_q7;
    ACC = retained_barometric_correction;
    clamp_stock_u8();
    retained_barometric_correction = ACC;

    ACC = retained_status;
    if (!startup_mode_15 && !ACC_BIT1 && !retained_ram_checksum_failed) {
        ACC = retain_startup_data_config;
        if (ACC_BIT0) {
            goto use_retained_data;
        }
    }

    volatile __xdata uint8_t *entry = retained_entries;
    B = sizeof(retained_entries);
    do {
        *entry++ = 0;
    } while (--B != 0);
    clear_retained_words_ce94();

use_retained_data:
    init_startup_baro_history();
}

void init_startup_baro_history(void)
{
    filter_retained_entries();
    ACC = 0;
    retained_entries[6] = ACC;
    retained_field_f518 = ACC;

    B = sizeof(barometric_history);
    ACC = retained_barometric_correction;
    volatile __xdata uint8_t *entry = barometric_history;
    do {
        *entry++ = ACC;
    } while (--B != 0);

    sample_startup_coolant();
}
