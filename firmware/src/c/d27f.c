#include "stock_registers.h"

static __sfr __at (0x92) DPSEL;
static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0x7f) serial_extended_length;

static volatile __data __at (0x04) uint8_t stock_r4;
static volatile __data __at (0x51) uint8_t serial_tx_count;
static volatile __idata __at (0xa6) uint8_t serial_response_address;
static volatile __xdata __at (0xf600) uint8_t serial_tx_buffer[256];
static volatile __xdata __at (0xf680) uint8_t serial_rx_buffer[128];

static volatile __xdata __at (0xf460) uint8_t identification_90[20];
static volatile __xdata __at (0xf474) uint8_t identification_98[8];
static volatile __xdata __at (0xf485) uint8_t diagnostic_record_a1[8];
static volatile __xdata __at (0xf48d) uint8_t diagnostic_record_a2[8];
static volatile __xdata __at (0xf495) uint8_t diagnostic_record_a3[11];

extern void copy_xdata_bytes(void);
extern void append_serial_checksum(void);
extern void write_eeprom_record_f460(void);
extern void write_eeprom_record_f474(void);
extern void write_eeprom_record_f485(void);
extern void write_eeprom_record_f48d(void);
extern void write_eeprom_record_f495(void);

void build_record_write_ack(void)
{
    const uint8_t saved_dpsel = DPSEL;
    DPSEL = 7;
    serial_tx_buffer[0] = 0x82;
    serial_tx_buffer[1] = serial_response_address;
    serial_tx_buffer[2] = 0x10;
    serial_tx_buffer[3] = 0x7b;
    serial_tx_buffer[4] = stock_r4;
    serial_tx_count = 5;
    DPSEL = saved_dpsel;
    append_serial_checksum();
}

#define STORE_RECORD(record, write_eeprom, selector)                          \
    do {                                                                      \
        const uint8_t saved_dpsel = DPSEL;                                    \
        const uint8_t payload_size = sizeof(record) - 1;                      \
        DPSEL = 4;                                                            \
        STOCK_DPTR = (uint16_t)(record);                                      \
        stock_r2 = payload_size;                                              \
        DPSEL = 3;                                                            \
        STOCK_DPTR = (uint16_t)(serial_rx_buffer +                            \
                               (serial_extended_length ? 6 : 5));             \
        copy_xdata_bytes();                                                   \
        (record)[payload_size] = 0;                                           \
        DPSEL = saved_dpsel;                                                  \
        write_eeprom();                                                       \
        stock_r4 = (selector);                                                \
        build_record_write_ack();                                             \
    } while (0)

void store_identification_90(void)
{
    STORE_RECORD(identification_90, write_eeprom_record_f460, 0x90);
}

void store_identification_98(void)
{
    STORE_RECORD(identification_98, write_eeprom_record_f474, 0x98);
}

/* The bytes for SETB and RET are also the operands and return of the
 * preceding function's final LCALL. The lowering rule shares those bytes.
 */
void set_flag_2f_7_d2f1(void)
{
    serial_extended_length = 1;
}

void store_diag_record_a1(void)
{
    STORE_RECORD(diagnostic_record_a1, write_eeprom_record_f485, 0xa1);
}

void store_diag_record_a2(void)
{
    STORE_RECORD(diagnostic_record_a2, write_eeprom_record_f48d, 0xa2);
}

void store_diag_record_a3(void)
{
    STORE_RECORD(diagnostic_record_a3, write_eeprom_record_f495, 0xa3);
}
