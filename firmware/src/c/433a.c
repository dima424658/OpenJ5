#include "stock_registers.h"

static __sfr __at (0xf0) B;
static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0x6e) air_conditioning_active;
static volatile __data __at (4) uint8_t stock_r4;
static volatile __data __at (0x4e) uint8_t coolant_nearest_index;
static volatile __data __at (0x55) uint8_t engine_rpm_40;
static volatile __data __at (0x57) uint8_t rpm_q4;
static volatile __data __at (0x6a) uint8_t idle_airflow_request;
static volatile __xdata __at (0xf848) uint8_t idle_transition_rpm_2;

extern __code const uint8_t idle_airflow_by_temp[39];
extern __code const uint8_t idle_ac_airflow_by_temp[39];
extern __code const uint8_t idle_airflow_rpm_gain_q8[32];
extern void interp_curve_q4(void);
extern void select_idle_or_fuel_cut(void);

void update_idle_airflow(void)
{
    /* The threshold is in tens of RPM. Compare in units of 40 RPM. */
    B = idle_transition_rpm_2 / 4;
    const uint8_t rpm = engine_rpm_40;
    if (rpm < B) {
        B = 0;
    } else {
        stock_r4 = rpm - B;
        STOCK_DPTR = (uint16_t)idle_airflow_rpm_gain_q8;
        stock_r2 = rpm_q4;
        interp_curve_q4();

        /* Divide by 128 to express the correction in airflow table units. */
        const uint16_t weighted_excess = (uint16_t)ACC * stock_r4;
        B = weighted_excess >= 0x8000u ? UINT8_MAX : weighted_excess >> 7;
    }

    if (air_conditioning_active) {
        ACC = idle_ac_airflow_by_temp[coolant_nearest_index];
    } else {
        ACC = idle_airflow_by_temp[coolant_nearest_index];
    }
    ACC += B;
    if (ACC < B) {
        ACC = UINT8_MAX;
    }
    idle_airflow_request = ACC;

    select_idle_or_fuel_cut();
}
