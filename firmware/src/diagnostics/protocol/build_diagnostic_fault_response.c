#include "stock_registers.h"

static __sfr __at (0x92) DPSEL;
static __sfr16 __at (0x8382) STOCK_DPTR;

static volatile __data __at (0x51) uint8_t serial_tx_count;
static volatile __idata __at (0x93) uint8_t diagnostic_fault_count;
static volatile __idata __at (0xa6) uint8_t serial_response_address;
static volatile __xdata __at (0xf400) uint8_t diagnostic_fault_records_start;
static volatile __xdata __at (0xf600) uint8_t serial_tx_buffer[256];

extern void append_serial_checksum(void);
extern void copy_xdata_bytes(void);

void build_diagnostic_fault_response(void)
{
    stock_r3 = diagnostic_fault_count;
    stock_r2 = (uint8_t)(3 * stock_r3);
    const uint8_t saved_dpsel = DPSEL;
    DPSEL = 7;
    serial_tx_buffer[0] = (uint8_t)(stock_r2 + 2u) | 0x80;
    serial_tx_buffer[1] = serial_response_address;
    serial_tx_buffer[2] = 0x10;
    serial_tx_buffer[3] = 0x58;
    serial_tx_buffer[4] = stock_r3;
    serial_tx_count = 5;
    if (stock_r3) {
        serial_tx_count = stock_r2 + 5u;
        DPSEL = 4;
        STOCK_DPTR = (uint16_t)&serial_tx_buffer[5];
        DPSEL = 3;
        STOCK_DPTR = (uint16_t)&diagnostic_fault_records_start;
        copy_xdata_bytes();
    }
    DPSEL = saved_dpsel;
    append_serial_checksum();
}
