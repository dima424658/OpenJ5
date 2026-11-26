#include "stock_registers.h"

static __sfr __at (0xb9) sfr_b9;
static __sfr __at (0xf0) B;
static __sfr __at (0xf8) P5;
static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0x0d) monitor_serial_error;
static __sbit __at (0xaf) EA;
static __sbit __at (0xd1) saved_interrupt_enable;
static __sbit __at (0xd7) CY;
static __sbit __at (0xff) serial_line;

static volatile __data __at (0x71) uint8_t monitor_serial_steps;
static volatile __data __at (0x72) uint8_t monitor_serial_crc;
static volatile __xdata __at (0xfc00) uint8_t monitor_serial_buffer[11];

extern void monitor_delay_acc(void);

#define DRIVE_LINE() do { sfr_b9 |= 0x80; P5 &= 0x7f; } while (0)
#define RELEASE_LINE() do { sfr_b9 |= 0x80; P5 |= 0x80; } while (0)
#define NOP() __asm nop __endasm

#define ROTATE_ACC_RIGHT()                                  \
    do {                                                    \
        const uint8_t high_bit = CY ? 0x80 : 0;             \
        CY = ACC & 1;                                       \
        ACC = (ACC >> 1) | high_bit;                        \
    } while (0)

#define WAIT_SLOT()                                         \
    do {                                                    \
        const uint8_t saved_acc = ACC;                      \
        ACC = 0x1b;                                         \
        monitor_delay_acc();                                \
        ACC = saved_acc;                                    \
    } while (0)

void reset_monitor_serial_line(void)
{
    DRIVE_LINE();
    serial_line = 0;
    ACC = 0xd6;
    monitor_delay_acc();
    serial_line = 1;
    RELEASE_LINE();

    B = 0x16;
    do {
        CY = serial_line;
        --B;
    } while (B != 0 && CY);

    if (B != 0) {
        B = 0x5b;
        do {
            CY = serial_line;
            --B;
        } while (B != 0 && !CY);
        if (B != 0) {
            ACC = 0xd5;
            do {
                NOP();
            } while (--ACC);
        }
        CY = !CY;
    }
    monitor_serial_error = CY;
}

void write_monitor_serial_byte(void)
{
    B = 8;
    DRIVE_LINE();
    do {
        NOP();
        NOP();
        serial_line = 0;
        ROTATE_ACC_RIGHT();
        NOP();
        NOP();
        serial_line = CY;
        WAIT_SLOT();
        serial_line = 1;
    } while (--B);
    RELEASE_LINE();
}

/* R0 is the running reflected CRC-8 remainder. */
void read_monitor_serial_byte(void)
{
    B = 8;
    saved_interrupt_enable = EA;
    do {
        DRIVE_LINE();
        EA = 0;
        serial_line = 0;
        RELEASE_LINE();
        serial_line = 1;
        CY = serial_line;
        ROTATE_ACC_RIGHT();
        EA = saved_interrupt_enable;

        const uint8_t sample = ACC;
        const uint8_t mix = stock_r0 ^ (sample >> 7);
        stock_r0 = (mix >> 1) ^ ((mix & 1) ? 0x8c : 0);
        CY = (mix & 1) != 0;
        WAIT_SLOT();
    } while (--B);
}

void clear_monitor_serial_buffer(void)
{
    STOCK_DPTR = (uint16_t)monitor_serial_buffer;
    B = 8;
    ACC = 0;
    do {
        *(volatile __xdata uint8_t *)STOCK_DPTR = ACC;
        ++STOCK_DPTR;
    } while (--B);
}

void read_monitor_serial_block(void)
{
    reset_monitor_serial_line();
    if (!CY) {
        stock_r0 = 0;
        stock_r1 = 8;
        ACC = 0x0f;
        write_monitor_serial_byte();
        do {
            read_monitor_serial_byte();
            *(volatile __xdata uint8_t *)STOCK_DPTR = ACC;
            ++STOCK_DPTR;
        } while (--stock_r1);
        if (stock_r0 == 0) {
            CY = 0;
            return;
        }
    }
    monitor_serial_error = 1;
    CY = 1;
}

void step_monitor_serial_block(void)
{
    ACC = monitor_serial_steps;
    if (ACC == 0 || ACC >= 12) {
        return;
    }
    --monitor_serial_steps;
    if (ACC == 10) {
        reset_monitor_serial_line();
    } else if (ACC == 9) {
        ACC = 0x0f;
        write_monitor_serial_byte();
        monitor_serial_crc = 0;
    } else {
        /* Step 11 fills slot 10. Steps 8 through 1 fill slots 7 through 0. */
        STOCK_DPTR = (uint16_t)monitor_serial_buffer - 1 + ACC;
        stock_r0 = monitor_serial_crc;
        read_monitor_serial_byte();
        monitor_serial_crc = stock_r0;
        *(volatile __xdata uint8_t *)STOCK_DPTR = ACC;
        CY = 0;
        if (monitor_serial_steps == 0 && stock_r0 != 0) {
            monitor_serial_error = 1;
        }
    }
}
