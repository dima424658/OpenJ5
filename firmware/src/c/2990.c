#include "stock_registers.h"

enum { STARTUP_IAC_STATE_VALID = 0x55 };

static volatile __data __at (0x49) uint8_t coolant_temperature_raw;
static volatile __data __at (0x64) uint8_t iac_target;
static volatile __data __at (0x65) uint8_t iac_position;
static volatile __xdata __at (0xf80a) uint8_t startup_coolant_temperature_raw;
static volatile __xdata __at (0xf845) uint16_t iac_feedback_position_q4;
static volatile __xdata __at (0xf8c2) uint8_t startup_iac_state_marker;
static volatile __xdata __at (0xf9af) uint8_t retained_iac_position;

extern volatile __code const uint8_t cold_start_coolant_threshold;
extern volatile __code const uint8_t cold_start_iac_max_position;
extern volatile __code const uint8_t iac_max_position;

extern void home_iac_at_startup(void);
extern void shift_left_saturating_4(void);
extern void continue_startup_after_iac(void);

void seed_startup_iac_feedback(void);

void home_and_seed_iac_feedback(void)
{
    home_iac_at_startup();
    seed_startup_iac_feedback();
}

void restore_startup_iac_position(void)
{
    startup_coolant_temperature_raw = coolant_temperature_raw;
    if (cold_start_coolant_threshold >= coolant_temperature_raw) {
        ACC = cold_start_iac_max_position;
    } else {
        ACC = iac_max_position;
    }
    iac_target = ACC;
    iac_position = retained_iac_position;
    seed_startup_iac_feedback();
}

void seed_startup_iac_feedback(void)
{
    stock_r01 = iac_target;
    shift_left_saturating_4();
    iac_feedback_position_q4 = stock_r01;
    startup_iac_state_marker = STARTUP_IAC_STATE_VALID;
    continue_startup_after_iac();
}
