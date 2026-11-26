#include "stock_registers.h"

static __sfr16 __at (0x8382) STOCK_DPTR;

static __sbit __at (0x00) monitor_command_active;
static __sbit __at (0x02) monitor_result_ready;
static __sbit __at (0x03) monitor_checksum_failed;
static __sbit __at (0xd7) CY;

static volatile __data __at (0x3e) uint8_t monitor_eeprom_request;
static volatile __data __at (0x62) uint8_t monitor_eeprom_device;
static volatile __xdata __at (0xff00) uint8_t monitor_eeprom_buffer[256];

extern void monitor_eeprom_readback_256(void);
extern void monitor_eeprom_write_and_verify(void);
extern void append_monitor_word_checksum(void);
extern void poll_monitor_exit(void);
extern void resume_service_monitor(void);

#define COMPLETE_EEPROM_REQUEST()                                         \
    do {                                                                  \
        monitor_checksum_failed = CY;                                     \
        monitor_command_active = 0;                                       \
        monitor_result_ready = 1;                                         \
        resume_service_monitor();                                         \
    } while (0)

void fill_or_read_monitor_eeprom(void)
{
    stock_r2 = 0;
    ACC = monitor_eeprom_request;
    if (ACC_SIGN) {
        STOCK_DPTR = (uint16_t)monitor_eeprom_buffer;
        stock_r0 = 0;
        monitor_eeprom_device = ACC & 0x7f;
        monitor_eeprom_readback_256();
    } else {
        monitor_eeprom_device = ACC;
        if (ACC != 0) {
            STOCK_DPTR = (uint16_t)monitor_eeprom_buffer + 0xe0;
            stock_r2 = 15;
            append_monitor_word_checksum();
            stock_r2 = 0xe0;
        }
        STOCK_DPTR = (uint16_t)monitor_eeprom_buffer;
        do {
            *(volatile __xdata uint8_t *)STOCK_DPTR = 0xff;
            ++STOCK_DPTR;
        } while (--stock_r2);

        STOCK_DPTR = (uint16_t)monitor_eeprom_buffer;
        stock_r0 = 0;
        stock_r2 = 0x80;
        monitor_eeprom_write_and_verify();
        if (!CY) {
            poll_monitor_exit();
            STOCK_DPTR = (uint16_t)monitor_eeprom_buffer + 0x80;
            stock_r0 = 0x80;
            stock_r2 = 0x80;
            monitor_eeprom_write_and_verify();
            if (!CY) {
                poll_monitor_exit();
            }
        }
    }
    COMPLETE_EEPROM_REQUEST();
}

#define CHECK_BLANK_EEPROM(device, count)                                 \
    do {                                                                  \
        monitor_eeprom_device = (device);                                 \
        monitor_eeprom_readback_256();                                    \
        if (CY) {                                                         \
            goto complete;                                                \
        }                                                                 \
        STOCK_DPTR = (uint16_t)monitor_eeprom_buffer;                     \
        stock_r1 = (count);                                               \
        do {                                                              \
            ACC = *(volatile __xdata uint8_t *)STOCK_DPTR;                \
            ++STOCK_DPTR;                                                 \
            if (ACC != 0xff) {                                            \
                CY = 1;                                                   \
                goto complete;                                            \
            }                                                             \
        } while (--stock_r1);                                             \
    } while (0)

void check_blank_monitor_eeprom(void)
{
    CHECK_BLANK_EEPROM(0, 0);  /* A zero counter scans all 256 bytes. */
    poll_monitor_exit();
    CHECK_BLANK_EEPROM(1, 0xe0);
    CY = 0;
complete:
    COMPLETE_EEPROM_REQUEST();
}
