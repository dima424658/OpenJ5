#include "stock_registers.h"

static __sfr __at (0xb2) sfr_b2;
static __sfr __at (0xfa) P6;
static __sbit __at (0x00) monitor_command_active;
static __sbit __at (0x05) monitor_request_invalid;
static __sbit __at (0x90) port1_bit0;
static __sbit __at (0x91) port1_bit1;
static __sbit __at (0xe8) port4_bit0;
static __sbit __at (0xe9) port4_bit1;
static __sbit __at (0xea) port4_bit2;
static __sbit __at (0xeb) port4_bit3;
static __sbit __at (0xed) port4_bit5;
static __sbit __at (0xee) port4_bit6;
static __sbit __at (0xef) port4_bit7;
static __sbit __at (0xf9) port5_bit1;
static __sbit __at (0xfa) port5_bit2;
static __sbit __at (0xfd) port5_bit5;
static __sbit __at (0xfe) port5_bit6;

static volatile __data __at (0x3f) uint8_t monitor_request_kind;
static volatile __data __at (0x51) uint8_t monitor_field_address;
static volatile __data __at (0x52) uint8_t monitor_field_mask;
static volatile __data __at (0x7e) uint8_t monitor_transfer_byte;

extern void poll_monitor_exit_and_io(void);
extern void resume_service_monitor(void);

#define SELECT_MONITOR_FIELD(address, mask)              \
    do {                                                 \
        monitor_field_address = (address);               \
        monitor_field_mask = (mask);                     \
    } while (0)

#define MIRROR_BIT(target) ((target) = (sfr_b2 & 2) == 0)

#define MIRROR_MASK(target, mask)                        \
    do {                                                 \
        if (sfr_b2 & 2) {                                \
            (target) |= (mask);                          \
        } else {                                         \
            (target) &= (uint8_t)~(mask);                \
        }                                                \
    } while (0)

#define MONITOR_MIRROR(subcode, setup, update)           \
    if (ACC == (subcode)) {                              \
        setup;                                           \
        for (;;) {                                       \
            update;                                      \
            poll_monitor_exit_and_io();                  \
        }                                                \
    }

void mirror_monitor_output(void)
{
    monitor_field_mask = 0;
    ACC = monitor_request_kind;

    MONITOR_MIRROR(0x01, (void)0, MIRROR_BIT(port5_bit1));
    MONITOR_MIRROR(0x03, SELECT_MONITOR_FIELD(0x80, 0x0c), MIRROR_MASK(monitor_transfer_byte, 0x02));
    MONITOR_MIRROR(0x04, (void)0, MIRROR_MASK(P6, 0x80));
    MONITOR_MIRROR(0x05, SELECT_MONITOR_FIELD(0x85, 0x07), MIRROR_BIT(port4_bit6));
    MONITOR_MIRROR(0x06, SELECT_MONITOR_FIELD(0x81, 0x30), MIRROR_MASK(monitor_transfer_byte, 0x40));
    MONITOR_MIRROR(0x10, SELECT_MONITOR_FIELD(0x83, 0x07), MIRROR_BIT(port4_bit1));
    MONITOR_MIRROR(0x11, SELECT_MONITOR_FIELD(0x85, 0x70), MIRROR_BIT(port4_bit7));
    MONITOR_MIRROR(0x14, (void)0, MIRROR_BIT(port5_bit2));
    MONITOR_MIRROR(0x15, (void)0, MIRROR_MASK(P6, 0x40));
    MONITOR_MIRROR(0x16, SELECT_MONITOR_FIELD(0x80, 0xc0), MIRROR_MASK(monitor_transfer_byte, 0x08));
    MONITOR_MIRROR(0x17, SELECT_MONITOR_FIELD(0x83, 0x70), MIRROR_BIT(port4_bit0));
    MONITOR_MIRROR(0x19, SELECT_MONITOR_FIELD(0x81, 0xc0), MIRROR_MASK(monitor_transfer_byte, 0x80));
    MONITOR_MIRROR(0x1a, (void)0, MIRROR_MASK(P6, 0x10));
    MONITOR_MIRROR(0x1f, SELECT_MONITOR_FIELD(0x84, 0x70), MIRROR_BIT(port1_bit1));
    MONITOR_MIRROR(0x21, (void)0, MIRROR_BIT(port5_bit6));
    MONITOR_MIRROR(0x22, SELECT_MONITOR_FIELD(0x82, 0x70), MIRROR_BIT(port4_bit3));
    MONITOR_MIRROR(0x23, SELECT_MONITOR_FIELD(0x82, 0x07), MIRROR_BIT(port4_bit2));
    MONITOR_MIRROR(0x24, SELECT_MONITOR_FIELD(0x84, 0x07), MIRROR_BIT(port1_bit0));
    MONITOR_MIRROR(0x26, SELECT_MONITOR_FIELD(0x80, 0x30), MIRROR_MASK(monitor_transfer_byte, 0x04));
    MONITOR_MIRROR(0x2b, SELECT_MONITOR_FIELD(0x80, 0x03), MIRROR_BIT(port5_bit5));
    MONITOR_MIRROR(0x2e, SELECT_MONITOR_FIELD(0x81, 0x0c), MIRROR_MASK(monitor_transfer_byte, 0x20));
    MONITOR_MIRROR(0x36, SELECT_MONITOR_FIELD(0x81, 0x03), MIRROR_BIT(port4_bit5));

    monitor_command_active = 0;
    monitor_request_invalid = 1;
    resume_service_monitor();
}
