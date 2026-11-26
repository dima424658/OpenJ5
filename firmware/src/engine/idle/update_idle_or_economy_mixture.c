#include "stock_registers.h"

static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0x59) idle_mode;
static volatile __data __at (0x57) uint8_t rpm_q4;
static volatile __data __at (0x5c) uint8_t charge_q4;
static volatile __data __at (0x60) uint8_t requested_mixture;
static volatile __xdata __at (0xf887) uint8_t coolant_index;
static volatile __xdata __at (0xf888) uint8_t coolant_fraction;

extern __code const uint8_t idle_mixture_by_coolant[39];
extern __code const uint8_t economy_mixture_raw[256];
extern void interp_bilinear_u8(void);
extern void interp_curve_u8(void);
extern void update_base_ignition(void);

void update_idle_or_economy_mixture(void)
{
    /* DPTR holds the table, R2 and R3 hold its coordinates, and A returns the byte. */
    if (idle_mode) {
        stock_r2 = coolant_index;
        stock_r3 = coolant_fraction;
        STOCK_DPTR = (uint16_t)idle_mixture_by_coolant;
        interp_curve_u8();
    } else {
        STOCK_DPTR = (uint16_t)economy_mixture_raw;
        stock_r2 = rpm_q4;
        stock_r3 = charge_q4;
        interp_bilinear_u8();
    }
    requested_mixture = ACC;

    update_base_ignition();
}
