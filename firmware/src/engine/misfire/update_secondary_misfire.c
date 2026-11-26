#include "stock_registers.h"

enum { CONDITIONS_MASK = 0x3f, MISFIRE_STATUS_LOCK = 0x04 };

static __sfr __at (0xf0) B;
static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0xd7) CY;
static __sbit __at (0xe0) ACC_BIT0;
static __sbit __at (0xe1) ACC_BIT1;
static __sbit __at (0xe2) ACC_BIT2;
static __sbit __at (0xe3) ACC_BIT3;
static __sbit __at (0xe4) ACC_BIT4;
static __sbit __at (0xe5) ACC_BIT5;
static __sbit __at (0x56) fuel_mode_flag_56;
static __sbit __at (0x5b) deceleration_fuel_cut;

static volatile __data __at (4) uint8_t stock_r4;
static volatile __data __at (0x49) uint8_t coolant_temperature_raw;
static volatile __data __at (0x57) uint8_t rpm_index_q4;
static volatile __data __at (0x5c) uint8_t charge_q4;
static volatile __xdata __at (0xfa7e) uint8_t secondary_misfire_conditions;
static volatile __xdata __at (0xf8ae) uint8_t vehicle_speed;
static volatile __xdata __at (0xf93d) uint8_t primary_misfire_conditions;
static volatile __xdata __at (0xf8ce) uint8_t misfire_map_value;
static volatile __xdata __at (0xfa6c) uint8_t misfire_control_fa6c;
static volatile __xdata __at (0xfa6d) uint8_t misfire_control_fa6d;

extern __code const uint8_t misfire_secondary_limits[3];
extern __code const uint8_t misfire_rpm_charge_map[256];

extern void interp_bilinear_u8(void);
extern void continue_after_misfire(void);
extern void misfire_event_not_pending(void);
extern void process_pending_misfire_event(void);

void update_misfire_rpm_charge_map(void);
void dispatch_misfire_event(void);

void update_secondary_misfire(void)
{
    stock_r4 = secondary_misfire_conditions;

    CY = misfire_secondary_limits[1] < coolant_temperature_raw;
    ACC = stock_r4;
    ACC_BIT0 = CY;
    stock_r4 = ACC;
    CY = misfire_secondary_limits[2] >= coolant_temperature_raw;
    ACC = stock_r4;
    ACC_BIT1 = CY;
    stock_r4 = ACC;
    CY = misfire_secondary_limits[0] < charge_q4;
    ACC = stock_r4;
    ACC_BIT2 = CY;
    stock_r4 = ACC;
    CY = vehicle_speed < 1;
    ACC = stock_r4;
    ACC_BIT3 = CY;
    stock_r4 = ACC;

    ACC_BIT4 = !deceleration_fuel_cut;
    stock_r4 = ACC;

    ACC = primary_misfire_conditions;
    CY = ACC_BIT1;
    ACC = stock_r4;
    ACC_BIT5 = CY;
    ACC &= CONDITIONS_MASK;
    CY = ACC == CONDITIONS_MASK;
    ACC_SIGN = CY;
    secondary_misfire_conditions = ACC;

    update_misfire_rpm_charge_map();
}

void update_misfire_rpm_charge_map(void)
{
    stock_r2 = rpm_index_q4;
    stock_r3 = charge_q4;
    STOCK_DPTR = (uint16_t)misfire_rpm_charge_map;
    interp_bilinear_u8();
    misfire_map_value = ACC;

    if (fuel_mode_flag_56 && (misfire_control_fa6d & MISFIRE_STATUS_LOCK) == 0) {
        dispatch_misfire_event();
        return;
    }
    continue_after_misfire();
}

void dispatch_misfire_event(void)
{
    ACC = misfire_control_fa6c;
    B = ACC;
    if ((ACC & 1) == 0) {
        misfire_event_not_pending();
        return;
    }

    misfire_control_fa6c = B & 0xfe;
    process_pending_misfire_event();
}
