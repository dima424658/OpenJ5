#include "stock_registers.h"

static __sfr16 __at (0x8382) STOCK_DPTR;
static volatile __data __at (4) uint8_t stock_r4;
static volatile __data __at (0x51) uint8_t serial_tx_count;
static volatile __xdata __at (0xf600) uint8_t serial_tx_buffer[256];

void append_serial_checksum(void)
{
    stock_r4 = 0;
    stock_r3 = serial_tx_count;
    STOCK_DPTR = (uint16_t)serial_tx_buffer;
    do {
        /* The last addition also leaves the checksum in the accumulator. */
        ACC = *(volatile __xdata uint8_t *)STOCK_DPTR + stock_r4;
        stock_r4 = ACC;
        ++STOCK_DPTR;
    } while (--stock_r3);
    *(volatile __xdata uint8_t *)STOCK_DPTR = ACC;
    ++serial_tx_count;
}
