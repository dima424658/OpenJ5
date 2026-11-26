#ifndef JANVAR_NEGATIVE_SERVICE_REPLY_H
#define JANVAR_NEGATIVE_SERVICE_REPLY_H

#include "stock_registers.h"

static __sfr16 __at (0x8382) STOCK_DPTR;

static volatile __data __at (0x04) uint8_t stock_r4;
static volatile __data __at (0x51) uint8_t serial_tx_count;
static volatile __idata __at (0xa6) uint8_t serial_response_address;
static volatile __idata __at (0xa7) uint8_t serial_request_service;
static volatile __xdata __at (0xf600) uint8_t serial_tx_buffer[256];

extern void reset_serial_transmit_buffer(void);

#define WRITE_NEGATIVE_SERVICE_REPLY(reason)                            \
    do {                                                                \
        reset_serial_transmit_buffer();                                 \
        STOCK_DPTR = (uint16_t)serial_tx_buffer;                        \
                                                                        \
        const uint8_t fields[] = {                                      \
            0x83, serial_response_address, 0x10, 0x7f, serial_request_service,\
        };                                                              \
        stock_r4 = 0;                                                   \
        for (uint8_t index = 0; index < sizeof(fields); ++index) {      \
            const uint8_t byte = fields[index];                         \
            *(volatile __xdata uint8_t *)STOCK_DPTR = byte;             \
            ++STOCK_DPTR;                                               \
            stock_r4 += byte;                                           \
        }                                                               \
        *(volatile __xdata uint8_t *)STOCK_DPTR = (reason);             \
        ++STOCK_DPTR;                                                   \
        ACC = stock_r4 + (reason);                                      \
        *(volatile __xdata uint8_t *)STOCK_DPTR = ACC;                  \
        serial_tx_count = 7;                                            \
    } while (0)

#endif
