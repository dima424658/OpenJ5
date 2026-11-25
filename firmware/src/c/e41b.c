#include "stock_registers.h"

static __sbit __at (0x00) monitor_command_active;
static __sbit __at (0x02) monitor_result_ready;
static __sbit __at (0x03) monitor_checksum_failed;
static __sbit __at (0xae) iac_tick_interrupt_enabled;
static __sbit __at (0xbe) iac_tick_high_priority;
static volatile __data __at (0x42) uint16_t monitor_checksum_difference;

extern volatile __code const uint16_t flash_checksum_expected;
extern void resume_service_monitor(void);

void check_monitor_flash_checksum(void)
{
    uint16_t address = 0;
    stock_r01 = 0;
    do {
        stock_r01 += *(const __code uint8_t *)address;
        ++address;
        iac_tick_interrupt_enabled = 1;
        iac_tick_high_priority = 1;
    } while (address != 0);

    const uint16_t difference = flash_checksum_expected - stock_r01;
    monitor_checksum_difference = difference;
    if (difference != 0) {
        monitor_checksum_failed = 1;
    }
    monitor_command_active = 0;
    monitor_result_ready = 1;
    resume_service_monitor();
}
