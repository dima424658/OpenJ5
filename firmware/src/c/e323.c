#include "stock_registers.h"

static __sfr __at (0xf0) B;
static __sbit __at (0xd7) CY;
static __sbit __at (0xae) iac_tick_interrupt_enabled;
static __sbit __at (0xbe) iac_tick_high_priority;
static __sbit __at (0x00) monitor_command_active;
static __sbit __at (0x01) monitor_flag_01;
static __sbit __at (0x02) monitor_result_ready;
static __sbit __at (0x03) monitor_checksum_failed;
static __sbit __at (0x05) monitor_request_invalid;

static volatile __data __at (0x41) uint8_t monitor_request_offset;
static volatile __data __at (0x50) uint8_t monitor_selected_field;
static volatile __data __at (0x51) uint8_t monitor_field_address;
static volatile __data __at (0x52) uint8_t monitor_field_mask;

extern void poll_monitor_serial(void);
extern void resume_service_monitor(void);
extern void dispatch_monitor_request(void);

#define ROTATE_RIGHT_THROUGH_CARRY()                   \
    do {                                               \
        const uint8_t carry_in = CY ? 0x80 : 0;        \
        CY = ACC & 1;                                  \
        ACC = (ACC >> 1) | carry_in;                   \
    } while (0)

#define EXCHANGE_ACC_AND_B()                           \
    do {                                               \
        const uint8_t previous = ACC;                  \
        ACC = B;                                       \
        B = previous;                                  \
    } while (0)

void wait_for_monitor_request(void)
{
    ACC = monitor_field_mask;
    B = ACC;
    if (ACC != 0) {
        stock_r0 = monitor_field_address;
        ACC = *((volatile __idata uint8_t *)stock_r0) & B;
        /* Shift the selected field once per zero below the mask's first bit. */
        for (;;) {
            EXCHANGE_ACC_AND_B();
            ROTATE_RIGHT_THROUGH_CARRY();
            if (CY) {
                break;
            }
            EXCHANGE_ACC_AND_B();
            ROTATE_RIGHT_THROUGH_CARRY();
        }
    }
    EXCHANGE_ACC_AND_B();
    monitor_selected_field = ACC;

    do {
        iac_tick_interrupt_enabled = 1;
        iac_tick_high_priority = 1;
        poll_monitor_serial();
        ACC = monitor_request_offset;
    } while (ACC == 0);

    if (ACC < 0x1e || ACC >= 0x2c) {
        monitor_command_active = 0;
        monitor_flag_01 = 0;
        monitor_result_ready = 0;
        monitor_checksum_failed = 0;
        monitor_request_invalid = 1;
        resume_service_monitor();
        return;
    }
    dispatch_monitor_request();
}
