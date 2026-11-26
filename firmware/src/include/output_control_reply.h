#ifndef JANVAR_OUTPUT_CONTROL_REPLY_H
#define JANVAR_OUTPUT_CONTROL_REPLY_H

#include "stock_registers.h"

static __sfr __at (0x92) DPSEL;

static volatile __data __at (0x04) uint8_t stock_r4;
static volatile __data __at (0x51) uint8_t serial_tx_count;
static volatile __idata __at (0xa6) uint8_t serial_response_address;
static volatile __xdata __at (0xf600) uint8_t serial_tx_buffer[256];

extern __code const uint8_t output_invalid_reply[];

extern void append_code_string_to_serial(void);
extern void append_serial_checksum(void);

#define WRITE_OUTPUT_REPLY_HEADER(format)                    \
    do {                                                     \
        DPSEL = 7;                                           \
        serial_tx_buffer[0] = (format);                      \
        serial_tx_buffer[1] = serial_response_address;       \
        serial_tx_buffer[2] = 0x10;                          \
        serial_tx_buffer[3] = 0x70;                          \
        serial_tx_buffer[4] = stock_r4;                      \
        serial_tx_buffer[5] = stock_r3;                      \
    } while (0)

#define APPEND_OUTPUT_ERROR_REPLY(table)                     \
    do {                                                     \
        const uint16_t address = (uint16_t)(table);          \
        stock_r3 = address >> 8;                             \
        stock_r4 = (uint8_t)address;                         \
        append_code_string_to_serial();                      \
    } while (0)

#endif
