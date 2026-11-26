#include "stock_registers.h"

static __sfr __at (0xf0) B;
static __sfr __at (0xf9) driver_bus_f9;
static __sbit __at (0xd5) reuse_driver_byte;
static __sbit __at (0xec) serial_clock;
static __sbit __at (0xfc) serial_data_in;
static __sbit __at (0xd7) CY;

enum { MONITOR_DRIVER_REPORT_END = 0x86 };

extern void wait_for_monitor_request(void);

#define NOP() __asm nop __endasm

#define DECODE_DRIVER_PAIR()                                              \
    do {                                                                  \
        ACC = (uint8_t)~ACC & 3;                                          \
        if (ACC == 3) {                                                   \
            ACC = 4;                                                      \
        }                                                                 \
    } while (0)

#define ROTATE_DRIVER_PAIR()                                              \
    do {                                                                  \
        ACC = stock_r1;                                                   \
        ACC = (ACC << 2) | (ACC >> 6);                                    \
        stock_r1 = ACC;                                                   \
        DECODE_DRIVER_PAIR();                                             \
    } while (0)

#define READ_TWO_DRIVER_BITS()                                            \
    do {                                                                  \
        stock_r1 = 2;                                                     \
        do {                                                              \
            CY = serial_data_in;                                          \
            serial_clock = 1;                                             \
            ACC = (ACC << 1) | CY;                                        \
            serial_clock = 0;                                             \
        } while (--stock_r1);                                             \
        DECODE_DRIVER_PAIR();                                             \
    } while (0)

void read_monitor_aux_status(void)
{
    if (serial_data_in) {
        driver_bus_f9 &= 0xfd;
        NOP();
        serial_clock = 1;
        NOP();
        serial_clock = 0;
        NOP();
        /* One captured byte supplies two consecutive report bytes. */
        reuse_driver_byte = 0;

        do {
            if (!reuse_driver_byte) {
                stock_r1 = 8;
                do {
                    CY = serial_data_in;
                    serial_clock = 1;
                    ACC = (ACC >> 1) | (CY ? 0x80 : 0);
                    serial_clock = 0;
                } while (--stock_r1);
                stock_r1 = ACC;
            }
            reuse_driver_byte = !reuse_driver_byte;

            ROTATE_DRIVER_PAIR();
            B = ACC;
            ROTATE_DRIVER_PAIR();
            ACC = (ACC << 4) | B;
            *((volatile __idata uint8_t *)stock_r0) = ACC;
        } while (++stock_r0 != MONITOR_DRIVER_REPORT_END);
    } else {
        driver_bus_f9 &= 0xfd;
        do {
            READ_TWO_DRIVER_BITS();
            B = ACC;
            READ_TWO_DRIVER_BITS();
            ACC = (ACC << 4) | B;
            *((volatile __idata uint8_t *)stock_r0) = ACC;
        } while (++stock_r0 != MONITOR_DRIVER_REPORT_END);
    }
    driver_bus_f9 |= 2;
    wait_for_monitor_request();
}
