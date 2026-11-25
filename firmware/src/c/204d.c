#include "stock_registers.h"

enum {
    RETAINED_MARKER = 0x55,
    EEPROM_ALTERNATE_MARKER = 0xaa,
    POWERDOWN_MARKER = 0xa5,
    POWERDOWN_MARKER_ADDRESS = 0xa8,
};

static __sfr __at (0x81) SP;
static __sfr __at (0x86) sfr_86;
static __sfr __at (0x90) P1;
static __sfr __at (0xa9) sfr_a9;
static __sfr __at (0xb0) P3;
static __sfr __at (0xb1) sfr_b1;
static __sfr __at (0xd0) PSW;
static __sfr __at (0xe8) P4;
static __sfr __at (0xf0) B;
static __sfr __at (0xf8) P5;
static __sfr __at (0xf9) P9;
static __sfr __at (0xfa) P6;
static __sbit __at (0x68) startup_serial_mode;
static __sbit __at (0x6a) extra_state_enabled;
static __sbit __at (0x6b) retained_startup_ready;
static __sbit __at (0xae) iac_tick_interrupt_enabled;
static __sbit __at (0xaf) EA;
static __sbit __at (0xbe) iac_tick_high_priority;

static volatile __data __at (7) uint8_t stock_r7;
static volatile __data __at (0x22) uint8_t diagnostic_flags[4];
static volatile __idata __at (POWERDOWN_MARKER_ADDRESS) uint8_t powerdown_marker;
static volatile __xdata __at (0xf4b7) uint8_t startup_state_f4b7;
static volatile __xdata __at (0xf4b9) uint8_t service_timeout_count;
static volatile __xdata __at (0xf4c5) uint8_t immobilizer_state;
static volatile __xdata __at (0xf8c2) uint8_t startup_iac_state_marker;
static volatile __xdata __at (0xf999) uint8_t relay_outputs;
static volatile __xdata __at (0xf9ac) uint8_t shutdown_eeprom_marker;
static volatile __xdata __at (0xf9b0) uint8_t retained_status;

extern void read_startup_fault_history(void);
extern void read_eeprom_shutdown_record(void);
extern void restore_eeprom_block_f4a6(void);
extern void restore_eeprom_block_f4a0(void);
extern void service_extra_state_b73f(void);
extern void update_relay_outputs(void);
extern void reset_saved_state_ac3c(void);
extern void continue_startup_sensor_test(void);
extern void restart_runtime_init(void);

void restore_startup_eeprom(void);
void reset_startup_diagnostics(void);

void restart_firmware(void)
{
    EA = 0;
    if (sfr_a9 & 0x40) {
        restart_runtime_init();
        return;
    }

    sfr_a9 = 0;
    sfr_86 = 0x80;
    iac_tick_interrupt_enabled = 1;
    iac_tick_high_priority = 1;
    sfr_b1 = 0;
    SP = 0xc2;
    PSW = 0;
    P1 = 0x7f;
    P3 = 0xff;
    P4 = 0xef;
    P5 = 0xdf;
    P6 = 0xff;
    P9 = 0xff;

    if (startup_iac_state_marker == RETAINED_MARKER && !(immobilizer_state & 0x20)) {
        retained_startup_ready = 1;
        restart_runtime_init();
        return;
    }
    restore_startup_eeprom();
}

void restore_startup_eeprom(void)
{
    read_startup_fault_history();
    read_eeprom_shutdown_record();
    if (shutdown_eeprom_marker != RETAINED_MARKER) {
        restore_eeprom_block_f4a6();
        restore_eeprom_block_f4a0();
    }
    startup_serial_mode = 1;
    startup_state_f4b7 = 0;

    if (extra_state_enabled) {
        B = shutdown_eeprom_marker;
        if (B != RETAINED_MARKER) {
            if (B != EEPROM_ALTERNATE_MARKER || (stock_r7 & 0x08)) {
                service_extra_state_b73f();
            }
            startup_serial_mode = 1;
        }
    }

    relay_outputs = 0x64;
    update_relay_outputs();
    reset_startup_diagnostics();
}

void reset_startup_diagnostics(void)
{
    diagnostic_flags[0] = 0;
    diagnostic_flags[1] = 0;
    diagnostic_flags[2] = 0;
    diagnostic_flags[3] = 0;
    shutdown_eeprom_marker = 0;
    retained_status = 0;
    if (startup_iac_state_marker != RETAINED_MARKER) {
        stock_r0 = POWERDOWN_MARKER_ADDRESS;
        if (powerdown_marker != POWERDOWN_MARKER) {
            retained_status |= 2;
            service_timeout_count = 0;
            reset_saved_state_ac3c();
        }
    }
    continue_startup_sensor_test();
}
