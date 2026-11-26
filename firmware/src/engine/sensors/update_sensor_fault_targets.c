#include "stock_registers.h"

enum { HOLD_BASE_IGNITION = 0x04 };

static __sfr16 __at (0x8382) STOCK_DPTR;
static volatile __data __at (0x57) uint8_t rpm_q4;
static volatile __data __at (0x5c) uint8_t charge_q4;
static volatile __data __at (0x7a) uint8_t ignition_control_7a;
static volatile __data __at (0x60) uint8_t requested_mixture;
static volatile __data __at (0x62) uint8_t base_ignition;

extern __code const int8_t sensor_fault_ignition_raw[256];
extern volatile __code const uint8_t sensor_fault_mixture_raw;
extern void interp_bilinear_u8(void);
extern void finish_sensor_fault(void);

void update_sensor_fault_targets(void)
{
    requested_mixture = sensor_fault_mixture_raw;
    if (!(ignition_control_7a & HOLD_BASE_IGNITION)) {
        STOCK_DPTR = (uint16_t)sensor_fault_ignition_raw;
        stock_r2 = rpm_q4;
        stock_r3 = charge_q4;
        interp_bilinear_u8();
        base_ignition = ACC;
    }

    finish_sensor_fault();
}
