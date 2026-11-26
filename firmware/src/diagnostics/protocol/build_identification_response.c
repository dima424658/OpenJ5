#include "stock_registers.h"

static __sbit __at (0x7f) serial_extended_length;
static volatile __data __at (0x04) uint8_t stock_r4;
static volatile __xdata __at (0xf680) uint8_t serial_rx_buffer[128];
static volatile __xdata __at (0xf460) uint8_t identification_90_start;
static volatile __xdata __at (0xf474) uint8_t identification_98_start;

extern __code const uint8_t identification_prefix[];
extern __code const char stock_ecu_number[];
extern __code const char stock_supplier_hardware[];
extern __code const char stock_supplier_software[];
extern __code const char stock_engine_type[];
extern __code const char stock_programming_date[];
extern __code const char stock_software_id[];
extern __code const uint8_t identification_91_reply[];
extern __code const uint8_t identification_92_reply[];
extern __code const uint8_t identification_94_reply[];
extern __code const uint8_t identification_97_reply[];
extern __code const uint8_t identification_99_reply[];
extern __code const uint8_t identification_9a_reply[];
extern __code const uint8_t identification_invalid_reply[];

extern void append_code_string_to_serial(void);
extern void append_xdata_string_to_serial(void);
extern void append_serial_checksum(void);
extern void build_identification_90(void);
extern void build_identification_98(void);

#define APPEND_FIELD(field, append)                          \
    do {                                                     \
        const uint16_t address = (uint16_t)(field);          \
        stock_r3 = address >> 8;                             \
        stock_r4 = (uint8_t)address;                         \
        append();                                            \
    } while (0)

void build_identification_response(void)
{
    stock_r4 = serial_rx_buffer[4 + (serial_extended_length ? 1 : 0)];
    switch (stock_r4) {
    case 0x80:
        APPEND_FIELD(identification_prefix, append_code_string_to_serial);
        APPEND_FIELD(&identification_90_start, append_xdata_string_to_serial);
        APPEND_FIELD(stock_ecu_number, append_code_string_to_serial);
        APPEND_FIELD(stock_supplier_hardware, append_code_string_to_serial);
        APPEND_FIELD(stock_supplier_software, append_code_string_to_serial);
        APPEND_FIELD(stock_engine_type, append_code_string_to_serial);
        APPEND_FIELD(&identification_98_start, append_xdata_string_to_serial);
        APPEND_FIELD(stock_programming_date, append_code_string_to_serial);
        APPEND_FIELD(stock_software_id, append_code_string_to_serial);
        append_serial_checksum();
        return;
    case 0x90:
        build_identification_90();
        return;
    case 0x91:
        APPEND_FIELD(identification_91_reply, append_code_string_to_serial);
        return;
    case 0x92:
        APPEND_FIELD(identification_92_reply, append_code_string_to_serial);
        return;
    case 0x94:
        APPEND_FIELD(identification_94_reply, append_code_string_to_serial);
        return;
    case 0x97:
        APPEND_FIELD(identification_97_reply, append_code_string_to_serial);
        return;
    case 0x98:
        build_identification_98();
        return;
    case 0x99:
        APPEND_FIELD(identification_99_reply, append_code_string_to_serial);
        return;
    case 0x9a:
        APPEND_FIELD(identification_9a_reply, append_code_string_to_serial);
        return;
    default:
        APPEND_FIELD(identification_invalid_reply, append_code_string_to_serial);
        return;
    }
}
