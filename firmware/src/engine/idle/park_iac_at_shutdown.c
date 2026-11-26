#include <stdint.h>

enum {
    IGNITION_SUPPLY_ADC_CHANNEL = 1,
    IAC_HOMING_OVERTRAVEL = 8,
};

static __sfr __at (0xe0) ACC;
static __sfr __at (0xf0) B;
static __sbit __at (0x45) output_state_flag_45;
static __sbit __at (0xaf) interrupts_enabled;
static __sbit __at (0xae) iac_tick_interrupt_enabled;
static __sbit __at (0xbe) iac_tick_high_priority;

/* The step bookkeeping and threshold deliberately use physical bank zero. */
static volatile __data __at (0x00) uint8_t bank_zero_shutdown_voltage;
static volatile __data __at (0x04) uint8_t bank_zero_previous_steps;
static volatile __data __at (0x64) uint8_t iac_target;
static volatile __data __at (0x65) uint8_t iac_position;
static volatile __xdata __at (0xf9af) uint8_t retained_iac_position;

extern volatile __code const uint8_t cold_start_iac_max_position;
extern volatile __code const uint8_t iac_max_position;
extern volatile __code const uint8_t shutdown_voltage_raw;

extern void read_adc_8bit(void);
extern void reset_after_supply_return(void);

void park_iac_at_shutdown(void)
{
    retained_iac_position = iac_position;
    output_state_flag_45 = 0;
    ACC = cold_start_iac_max_position + IAC_HOMING_OVERTRAVEL;
    bank_zero_previous_steps = ACC;

    interrupts_enabled = 0;
    iac_position = ACC;
    iac_target = 0;
    interrupts_enabled = 1;

    /* Count the steps towards the seat against the retained position. */
    do {
        iac_tick_interrupt_enabled = 1;
        iac_tick_high_priority = 1;

        ACC = iac_position;
        if (ACC != bank_zero_previous_steps) {
            bank_zero_previous_steps -= ACC;
            ACC = retained_iac_position;
            if (ACC != 0) {
                ACC = ACC < bank_zero_previous_steps
                    ? 0 : ACC - bank_zero_previous_steps;
                retained_iac_position = ACC;
                bank_zero_previous_steps = iac_position;
            }
        }

        bank_zero_shutdown_voltage = shutdown_voltage_raw;
        B = IGNITION_SUPPLY_ADC_CHANNEL;
        read_adc_8bit();
        if (ACC >= bank_zero_shutdown_voltage) {
            break;
        }
        ACC = iac_target;
    } while (ACC != iac_position);

    /* After seating or supply recovery, return to the calibrated opening. */
    interrupts_enabled = 0;
    iac_target = iac_max_position;
    iac_position = retained_iac_position;
    interrupts_enabled = 1;

    do {
        iac_tick_interrupt_enabled = 1;
        iac_tick_high_priority = 1;
        retained_iac_position = iac_position;

        bank_zero_shutdown_voltage = shutdown_voltage_raw;
        B = IGNITION_SUPPLY_ADC_CHANNEL;
        read_adc_8bit();
        if (ACC >= bank_zero_shutdown_voltage) {
            goto check_supply_again;
        }
        ACC = iac_target;
    } while (ACC != iac_position);
    retained_iac_position = iac_position;

check_supply_again:
    bank_zero_shutdown_voltage = shutdown_voltage_raw;
    B = IGNITION_SUPPLY_ADC_CHANNEL;
    read_adc_8bit();
    if (ACC >= bank_zero_shutdown_voltage) {
        reset_after_supply_return();
    }
}
