#include "stock_registers.h"

static __sfr __at (0x92) DPSEL;
static __sfr16 __at (0x8382) STOCK_DPTR;

static volatile __data __at (0x04) uint8_t stock_r4;
static volatile __data __at (0x51) uint8_t serial_tx_count;
static volatile __idata __at (0xa6) uint8_t serial_response_address;
static volatile __xdata __at (0xf600) uint8_t serial_tx_buffer[256];

extern void append_xdata_string_to_serial(void);
extern void append_serial_checksum(void);
extern void copy_xdata_bytes(void);

#define WRITE_DIAG_HEADER(format)                            \
    do {                                                     \
        DPSEL = 7;                                           \
        serial_tx_buffer[0] = (format);                      \
        serial_tx_buffer[1] = serial_response_address;       \
        serial_tx_buffer[2] = 0x10;                          \
        serial_tx_buffer[3] = 0x61;                          \
        serial_tx_buffer[4] = stock_r4;                      \
    } while (0)

#define COPY_DIAG_RECORD(offset, size)                       \
    do {                                                     \
        const uint8_t saved = DPSEL;                         \
        WRITE_DIAG_HEADER((uint8_t)(0x82u + (size)));        \
        DPSEL = 4;                                           \
        STOCK_DPTR = (uint16_t)serial_tx_buffer + 5;         \
        stock_r2 = (size);                                   \
        DPSEL = 3;                                           \
        STOCK_DPTR = (uint16_t)(offset);                     \
        copy_xdata_bytes();                                  \
        serial_tx_count = 5 + (size);                        \
        DPSEL = saved;                                       \
        append_serial_checksum();                            \
    } while (0)

#define APPEND_DIAG_STRING(offset, format)                   \
    do {                                                     \
        const uint8_t saved = DPSEL;                         \
        WRITE_DIAG_HEADER(format);                           \
        STOCK_DPTR = (uint16_t)serial_tx_buffer + 5;         \
        serial_tx_count = 5;                                 \
        DPSEL = saved;                                       \
        stock_r3 = (offset) >> 8;                            \
        stock_r4 = (uint8_t)(offset);                        \
        append_xdata_string_to_serial();                     \
        append_serial_checksum();                            \
    } while (0)

void build_diag_record_a0(void)
{
    COPY_DIAG_RECORD(0xf47c, 4);
}

void build_diag_record_e0(void)
{
    COPY_DIAG_RECORD(0xf480, 5);
}

void build_diag_record_a1(void)
{
    APPEND_DIAG_STRING(0xf485, 0x89);
}

void build_diag_record_a2(void)
{
    APPEND_DIAG_STRING(0xf48d, 0x89);
}

void build_diag_record_a3(void)
{
    APPEND_DIAG_STRING(0xf495, 0x8c);
}
