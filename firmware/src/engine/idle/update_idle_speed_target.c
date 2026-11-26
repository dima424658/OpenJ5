#include "stock_registers.h"

enum { HOLD_IDLE_SPEED_TARGET = 0x40 };

static __sfr __at (0xf0) B;
static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0x0c) speed_sensor_fitted;
static __sbit __at (0x2e) speed_sensor_fault;
static __sbit __at (0x6d) vehicle_moving;
static __sbit __at (0x6e) air_conditioning_active;
static volatile __data __at (0x7a) uint8_t idle_control_7a;
static volatile __xdata __at (0xf847) uint8_t idle_transition_rpm_1;
static volatile __xdata __at (0xf848) uint8_t idle_transition_rpm_2;
static volatile __xdata __at (0xf849) uint8_t idle_rpm_target;
static volatile __xdata __at (0xf84f) uint8_t idle_rpm_correction;
static volatile __xdata __at (0xf887) uint8_t idle_temp_index;
static volatile __xdata __at (0xf888) uint8_t idle_temp_fraction;

extern __code const uint8_t desired_idle_rpm[39];
extern volatile __code const uint8_t idle_transition_1_ac_q8;
extern volatile __code const uint8_t idle_transition_1_q8;
extern volatile __code const uint8_t idle_transition_2_q8;
extern volatile __code const uint8_t moving_idle_rpm_offset;
extern void interp_curve_u8(void);
extern void update_idle_ignition(void);

void update_idle_transition_rpm(void);
void apply_moving_idle_rpm_offset(void);

// A and B carry the operands in the stock block. The sum saturates at 255.
#define ADD_IDLE_RPM_IN_ACC()                    \
    do {                                         \
        ACC += B;                                \
        if (ACC < B) {                           \
            ACC = UINT8_MAX;                     \
        }                                        \
    } while (0)

void update_idle_speed_target(void)
{
    if (idle_control_7a & HOLD_IDLE_SPEED_TARGET) {
        ACC = idle_rpm_target;
    } else {
        stock_r2 = idle_temp_index;
        stock_r3 = idle_temp_fraction;
        STOCK_DPTR = (uint16_t)desired_idle_rpm;
        interp_curve_u8();
        B = ACC;
        ACC = idle_rpm_correction;
        ADD_IDLE_RPM_IN_ACC();
    }
    update_idle_transition_rpm();
}

void update_idle_transition_rpm(void)
{
    stock_r0 = ACC;
    B = ACC;

    if (air_conditioning_active) {
        ACC = idle_transition_1_ac_q8;
    } else {
        ACC = idle_transition_1_q8;
    }
    B = ((uint16_t)ACC * B) >> 8;
    ACC = stock_r0;
    ADD_IDLE_RPM_IN_ACC();
    idle_transition_rpm_1 = ACC;

    B = ACC;
    ACC = idle_transition_2_q8;
    B = ((uint16_t)ACC * B) >> 8;
    ACC = idle_transition_rpm_1;
    ADD_IDLE_RPM_IN_ACC();
    idle_transition_rpm_2 = ACC;

    apply_moving_idle_rpm_offset();
}

void apply_moving_idle_rpm_offset(void)
{
    B = 0;
    if (!(idle_control_7a & HOLD_IDLE_SPEED_TARGET)
            && (!speed_sensor_fitted || speed_sensor_fault || vehicle_moving)) {
        B = moving_idle_rpm_offset;
    }
    ACC = stock_r0;
    ADD_IDLE_RPM_IN_ACC();
    idle_rpm_target = ACC;

    update_idle_ignition();
}
