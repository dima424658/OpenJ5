#include "stock_registers.h"

enum { HOLD_BASE_IGNITION = 0x04 };

static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0x02) egr_selected;
static volatile __data __at (0x57) uint8_t rpm_q4;
static volatile __data __at (0x5c) uint8_t charge_q4;
static volatile __data __at (0x7a) uint8_t ignition_control_7a;
static volatile __data __at (0x62) uint8_t base_ignition;

extern __code const int8_t economy_ignition_half_degrees[256];
extern __code const int8_t egr_ignition_half_degrees[256];
extern void interp_bilinear_u8(void);
extern void update_injection_phase(void);

void update_base_ignition(void)
{
    STOCK_DPTR = (uint16_t)economy_ignition_half_degrees;
    if (egr_selected) {
        STOCK_DPTR = (uint16_t)egr_ignition_half_degrees;
    }
    if (!(ignition_control_7a & HOLD_BASE_IGNITION)) {
        stock_r2 = rpm_q4;
        stock_r3 = charge_q4;
        interp_bilinear_u8();
        base_ignition = ACC;
    }

    update_injection_phase();
}
