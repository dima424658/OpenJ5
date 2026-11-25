#include "stock_registers.h"

enum { SPEED_WINDOW_TICKS = 30 };

static __sfr __at (0xf0) B;
static __sbit __at (0x0c) speed_sensor_fitted;
static __sbit __at (0x2e) speed_sensor_fault;
static __sbit __at (0x49) tick_1s;
static __sbit __at (0x6d) vehicle_moving;

static volatile __data __at (0x59) uint8_t rpm_nearest_bin;
static volatile __data __at (0x5e) uint8_t charge_nearest_bin;
static volatile __data __at (0x7f) uint8_t speed_pulse_count;
static volatile __xdata __at (0xf8ad) uint8_t speed_window_ticks;
static volatile __xdata __at (0xf8ae) uint8_t vehicle_speed;
static volatile __xdata __at (0xf8c0) uint8_t missing_speed_seconds;

extern volatile __code const uint8_t moving_speed_low;
extern volatile __code const uint8_t moving_speed_high;
extern volatile __code const uint8_t speed_diagnostic_rpm_bin;
extern volatile __code const uint8_t speed_diagnostic_charge_bin;
extern volatile __code const uint8_t speed_diagnostic_seconds;

extern void continue_after_speed_update(void);

void check_speed_sensor(void);
void clear_missing_speed_timer(void);
void advance_missing_speed_timer(void);

void update_vehicle_speed(void)
{
    ACC = speed_window_ticks + 1;
    speed_window_ticks = ACC;
    if (ACC == SPEED_WINDOW_TICKS) {
        speed_window_ticks = 0;
        // The stock exchange reads and clears the interrupt-driven counter atomically.
        ACC = speed_pulse_count;
        speed_pulse_count = 0;
        vehicle_speed = ACC;
        B = ACC;
        ACC = moving_speed_low;
        if (!vehicle_moving) {
            ACC = moving_speed_high;
        }
        vehicle_moving = ACC < B;
    }
    check_speed_sensor();
}

void check_speed_sensor(void)
{
    if (!speed_sensor_fitted) {
        continue_after_speed_update();
        return;
    }
    if (vehicle_moving) {
        speed_sensor_fault = 0;
        clear_missing_speed_timer();
        return;
    }
    if (speed_sensor_fault) {
        continue_after_speed_update();
        return;
    }

    B = speed_diagnostic_rpm_bin;
    if (rpm_nearest_bin < B) {
        clear_missing_speed_timer();
        return;
    }
    B = speed_diagnostic_charge_bin;
    if (charge_nearest_bin < B) {
        clear_missing_speed_timer();
        return;
    }
    advance_missing_speed_timer();
}

void clear_missing_speed_timer(void)
{
    missing_speed_seconds = 0;
    continue_after_speed_update();
}

void advance_missing_speed_timer(void)
{
    if (tick_1s) {
        B = speed_diagnostic_seconds;
        ACC = missing_speed_seconds + 1;
        missing_speed_seconds = ACC;
        if (ACC == B) {
            speed_sensor_fault = 1;
        }
    }
    continue_after_speed_update();
}
