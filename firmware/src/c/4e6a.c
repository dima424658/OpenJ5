#include "stock_registers.h"

enum {
    CELL_COLUMN_MASK = 0x0f,
    LEARNED_CORRECTION_MIN = 0x40,
    LEARNED_CORRECTION_MAX = 0xc0,
};

static __sfr __at (0x82) DPL;
static __sfr __at (0x83) DPH;
static __sfr16 __at (0x8382) STOCK_DPTR;
static __sfr __at (0xf0) B;
static __sbit __at (0x45) knock_rpm_cutoff_active;
static __sbit __at (0x5f) learning_sweep_active;
static __sbit __at (0x64) learning_purge_table_selected;

static volatile __data __at (0x5f) uint8_t operating_point_index;
static volatile __data __at (0x6e) uint8_t oxygen_control_adjustment;
static volatile __xdata __at (0xf889) uint8_t learning_cell_index;
static volatile __xdata __at (0xf88c) uint8_t learning_center_correction;
static volatile __xdata __at (0xf88e) uint8_t learning_center_index;
static volatile __xdata __at (0xfd00) uint8_t learned_correction_pages[512];

extern volatile __code const uint8_t fuel_learning_gradient;
extern void abs_accumulator_i8(void);
extern void clamp_stock_u8(void);
extern void continue_after_fuel_learning(void);

void update_one_fuel_learning_cell(void);

void continue_after_speed_update(void)
{
    if (knock_rpm_cutoff_active || !learning_sweep_active) {
        continue_after_fuel_learning();
        return;
    }
    update_one_fuel_learning_cell();
}

void update_one_fuel_learning_cell(void)
{
    stock_r0 = learning_cell_index;
    B = stock_r0 & CELL_COLUMN_MASK;
    stock_r1 = learning_center_index;
    ACC = (stock_r1 & CELL_COLUMN_MASK) - B;
    abs_accumulator_i8();
    B = ACC;

    stock_r0 >>= 4;
    ACC = (stock_r1 >> 4) - stock_r0;
    abs_accumulator_i8();
    if (ACC < B) {
        ACC = B;
    }

    // The byte product sets the correction range at this distance from the center.
    B = (uint8_t)(fuel_learning_gradient * ACC);
    const uint8_t center_for_lower = learning_center_correction;
    stock_r0 = center_for_lower >= B ? center_for_lower - B : 0;
    const uint8_t center_for_upper = learning_center_correction;
    const uint8_t upper = center_for_upper + B;
    stock_r1 = upper >= B ? upper : UINT8_MAX;

    ACC = learning_cell_index;
    STOCK_DPTR = (uint16_t)learned_correction_pages;
    DPL = ACC;
    if (learning_purge_table_selected) {
        ++DPH;
    }
    volatile __xdata uint8_t *entry = (volatile __xdata uint8_t *)STOCK_DPTR;
    ACC = *entry;
    stock_r2 = ACC;
    clamp_stock_u8();
    stock_r0 = LEARNED_CORRECTION_MIN;
    stock_r1 = LEARNED_CORRECTION_MAX;
    clamp_stock_u8();
    *entry = ACC;
    B = ACC;

    if (DPL == operating_point_index) {
        oxygen_control_adjustment += stock_r2 - B;
    }

    ACC = learning_cell_index + 1;
    learning_cell_index = ACC;
    if (ACC == 0) {
        learning_sweep_active = 0;
    }
    continue_after_fuel_learning();
}
