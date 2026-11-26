#include <stdint.h>

enum {
    LAMP_OVERRIDE = 0x01,
    MISFIRE_PRESENT = 0x80,
    CYLINDER_CUT_MASK = 0xf0,
    FLASH_PHASE = 0x80,
    FLASH_PHASE_TICKS = 25,
};

static __sfr __at (0xe0) ACC;
static __sfr __at (0xf0) B;
static __sbit __at (0xd7) CY;
static __sbit __at (0xe4) ACC_LAMP_OUTPUT;
static __sbit __at (0x56) fuel_mode_flag_56;
static __sbit __at (0x70) lamp_off_request;

static volatile __data __at (0x79) uint8_t output_override_flags;
static volatile __xdata __at (0xf4c9) uint8_t lamp_flash_counter;
static volatile __xdata __at (0xf807) uint8_t engine_run_seconds;
static volatile __xdata __at (0xf8a8) uint8_t sensor_state_previous;
static volatile __xdata __at (0xf8b0) uint8_t sensor_state_current;
static volatile __xdata __at (0xf999) uint8_t relay_output_latch;
static volatile __xdata __at (0xf9b0) uint8_t retained_status;
static volatile __xdata __at (0xfa6d) uint8_t misfire_status;

extern volatile __code const uint8_t diagnostic_lamp_delay_seconds;

extern void age_fault_history_timers(void);
extern void collect_fault_status_records(void);
extern __bit has_confirmed_fault(void);
extern void continue_shutdown_outputs(void);

void update_lamp_and_outputs(void)
{
    age_fault_history_timers();
    collect_fault_status_records();

    if (!(output_override_flags & LAMP_OVERRIDE)) {
        CY = 1;
        if (!fuel_mode_flag_56) {
            // The pre-run lamp check sets the output bit without using bit 70.
            lamp_off_request = 1;
            CY = !CY;
            goto publish;
        }

        B = diagnostic_lamp_delay_seconds;
        if (engine_run_seconds >= B) {
            lamp_off_request = 1;
            if (has_confirmed_fault()) {
                if ((retained_status & MISFIRE_PRESENT) &&
                    !(misfire_status & CYLINDER_CUT_MASK)) {
                    uint8_t next = lamp_flash_counter + 1;
                    B = next;
                    if (next & FLASH_PHASE) {
                        lamp_off_request = 0;
                        if ((next & ~FLASH_PHASE) >= FLASH_PHASE_TICKS) {
                            B = 0;
                        }
                    } else if (next >= FLASH_PHASE_TICKS) {
                        B = FLASH_PHASE;
                    }
                    lamp_flash_counter = B;
                } else {
                    lamp_flash_counter = 0;
                    lamp_off_request = 0;
                }
            }
        }
    }

    CY = lamp_off_request;

publish:
    CY = !CY;
    ACC = relay_output_latch;
    ACC_LAMP_OUTPUT = CY;
    relay_output_latch = ACC;
    sensor_state_previous = sensor_state_current;
    continue_shutdown_outputs();
}
