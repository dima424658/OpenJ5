#include "stock_registers.h"

static __sfr __at (0x80) P0;
static __sfr __at (0x90) P1;
static __sfr __at (0xa0) P2;
static __sfr __at (0xb0) P3;
static __sfr __at (0xe8) P4;
static __sfr __at (0xf0) B;
static __sfr __at (0xf8) P5;
static __sfr __at (0xf9) P9;
static __sfr __at (0xfa) P6;
static __sbit __at (0xd7) CY;
static __sbit __at (0xaf) EA;
static __sbit __at (0x00) monitor_command_active;
static __sbit __at (0x01) monitor_flag_01;
static __sbit __at (0x02) monitor_result_ready;
static __sbit __at (0x03) monitor_checksum_failed;
static __sbit __at (0x05) monitor_request_invalid;

static volatile __data __at (0x40) uint8_t monitor_port_mode;
static volatile __data __at (0x61) uint8_t alternate_startup_marker;
static volatile __data __at (0x7e) uint8_t monitor_transfer_bytes[2];

extern void check_monitor_flash_checksum(void);
extern void toggle_monitor_output(void);
extern void monitor_handler_e470(void);
extern void monitor_handler_e665(void);
extern void monitor_handler_e66c(void);
extern void monitor_handler_e6b0(void);
extern void monitor_handler_e7a9(void);
extern void monitor_handler_eb0f(void);
extern void monitor_handler_e7c0(void);
extern void monitor_handler_e978(void);
extern void monitor_handler_ebc1(void);
extern void monitor_handler_ea58(void);
extern void monitor_handler_eae0(void);
extern void monitor_handler_eaab(void);

#define ROTATE_RIGHT_THROUGH_CARRY()                 \
    do {                                             \
        const uint8_t carry_in = CY ? 0x80 : 0;      \
        CY = ACC & 1;                                \
        ACC = (ACC >> 1) | carry_in;                 \
    } while (0)

#define MONITOR_COMMAND(index, handler)              \
    do {                                             \
        if (stock_r0 == (index)) {                   \
            handler();                               \
            return;                                  \
        }                                            \
    } while (0)

void dispatch_monitor_request(void)
{
    EA = 0;
    stock_r0 = ACC;
    ACC = monitor_port_mode;
    if (ACC == 0) {
        P0 = 0xff;
        P1 = 0xef;
        P2 = 0xff;
        P3 = 0xff;
        P4 = 0xef;
        P5 = 0xff;
        P6 = 0x2f;
        P9 = 0x66;
        monitor_transfer_bytes[0] = 0;
    } else {
        P0 = 0xff;
        P1 = 0xec;
        P2 = 0xff;
        P3 = 0xff;
        P4 = 0;
        P5 = 0x99;
        P6 = 0xff;
        P9 = 0x66;
        monitor_transfer_bytes[0] = 0xee;
    }

    const uint8_t request = stock_r0;
    stock_r0 = (uint8_t)(uint16_t)monitor_transfer_bytes;
    P9 &= 0xfb;
    ACC = *((volatile __idata uint8_t *)stock_r0);
    ++stock_r0;
    *((volatile __idata uint8_t *)stock_r0) = ACC;
    B = 8;
    do {
        ROTATE_RIGHT_THROUGH_CARRY();
        if (CY) {
            P9 |= 0x08;
        } else {
            P9 &= 0xf7;
        }
        P9 |= 1;
        CY = (P9 & 0x40) != 0;
        P9 &= 0xfe;
    } while (--B);
    ROTATE_RIGHT_THROUGH_CARRY();
    ++stock_r0;
    P9 |= 4;
    stock_r0 = request;

    monitor_command_active = 1;
    monitor_flag_01 = 0;
    monitor_result_ready = 0;
    monitor_checksum_failed = 0;
    monitor_request_invalid = 0;
    if (alternate_startup_marker != 0x55) {
        monitor_handler_e978();
        return;
    }

    stock_r0 -= 0x1e;
    MONITOR_COMMAND(0, check_monitor_flash_checksum);
    MONITOR_COMMAND(1, toggle_monitor_output);
    MONITOR_COMMAND(2, monitor_handler_e470);
    MONITOR_COMMAND(3, monitor_handler_e665);
    MONITOR_COMMAND(4, monitor_handler_e66c);
    MONITOR_COMMAND(5, monitor_handler_e6b0);
    MONITOR_COMMAND(6, monitor_handler_e7a9);
    MONITOR_COMMAND(7, monitor_handler_eb0f);
    MONITOR_COMMAND(8, monitor_handler_e7c0);
    MONITOR_COMMAND(9, monitor_handler_e978);
    MONITOR_COMMAND(10, monitor_handler_ebc1);
    MONITOR_COMMAND(11, monitor_handler_ea58);
    MONITOR_COMMAND(12, monitor_handler_eae0);
    MONITOR_COMMAND(13, monitor_handler_eaab);
}
