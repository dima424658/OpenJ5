#include "retained_ram.h"

enum {
    SERVICE_EEPROM_MARKER = 0xff,
    SERVICE_STATE_SAVED = 0x55,
};

static __sfr __at (0x86) sfr_86;
static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0xe5) ACC_BIT5;
static __sbit __at (0x45) service_mode_45;
static __sbit __at (0x56) service_park_iac_56;
static __sbit __at (0x69) service_inhibit_69;
static __sbit __at (0x79) service_request_79;
static __sbit __at (0xaf) interrupts_enabled;
static __sbit __at (0xae) iac_tick_interrupt_enabled;
static __sbit __at (0xbe) iac_tick_high_priority;

static volatile __data __at (0x64) uint8_t iac_target;
static volatile __data __at (0x65) uint8_t iac_position;
static volatile __idata __at (0x91) uint8_t service_status_91;
static volatile __xdata __at (0xf9a3) uint8_t service_saved_marker;
static volatile __xdata __at (0xf9af) uint8_t retained_iac_position;
static volatile __xdata __at (0xfa43) uint8_t service_eeprom_block[8];

extern volatile __code const uint8_t iac_max_position;

extern void run_service_mode_53b1(void);
extern void continue_service_normal(void);
extern void stop_engine_outputs(void);
extern void write_eeprom_status_marker(void);
extern void reset_saved_state_ac3c(void);
extern void reset_saved_state_ac95(void);
extern void write_eeprom_shutdown_block(void);
extern void write_eeprom_diagnostic_state(void);
extern void write_eeprom_shutdown_words(void);

void continue_service_cycle(void);
void park_iac_for_service(void);
void save_service_state(void);
void save_service_ram_checksum(void);

void check_service_save_request(void)
{
    if (service_inhibit_69 || !service_request_79) {
        goto continue_cycle;
    }
    ACC = service_status_91;
    if (!ACC_BIT5) {
        goto continue_cycle;
    }
    if (service_park_iac_56) {
        park_iac_for_service();
        return;
    }
    save_service_state();
    return;

continue_cycle:
    continue_service_cycle();
}

void park_iac_for_service(void)
{
    iac_target = iac_max_position;
    sfr_86 = 0x80;
    do {
        iac_tick_interrupt_enabled = 1;
        iac_tick_high_priority = 1;
        ACC = iac_position ^ iac_target;
    } while (ACC != 0);
    retained_iac_position = iac_position;
    stop_engine_outputs();
    save_service_state();
}

void save_service_state(void)
{
    interrupts_enabled = 0;
    ACC = SERVICE_EEPROM_MARKER;
    write_eeprom_status_marker();
    reset_saved_state_ac3c();
    reset_saved_state_ac95();
    STOCK_DPTR = (uint16_t)service_eeprom_block;
    write_eeprom_shutdown_block();
    write_eeprom_diagnostic_state();
    write_eeprom_shutdown_words();
    service_saved_marker = SERVICE_STATE_SAVED;
    save_service_ram_checksum();
}

void save_service_ram_checksum(void)
{
    ACCUMULATE_RETAINED_RAM();
    retained_ram_checksum = stock_r01;

    sfr_86 = 0x7f;
    iac_tick_interrupt_enabled = 1;
    iac_tick_high_priority = 1;
    for (;;) {
    }
}

void continue_service_cycle(void)
{
    if (service_mode_45) {
        run_service_mode_53b1();
        return;
    }
    continue_service_normal();
}
