#include "stock_registers.h"

static __sfr __at (0x92) DPSEL;
static __sfr16 __at (0x8382) STOCK_DPTR;

static volatile __data __at (0x04) uint8_t stock_r4;
static volatile __data __at (0x51) uint8_t serial_tx_count;
static volatile __idata __at (0xa6) uint8_t serial_response_address;
static volatile __xdata __at (0xf600) uint8_t serial_tx_buffer[256];

extern __code const uint8_t diag_full_iram[];
extern __code const uint8_t diag_full_xdata_bytes[][2];
extern __code const uint8_t diag_full_xdata_words[][2];
extern __code const uint8_t diag_short_iram[];
extern __code const uint8_t diag_short_xdata_bytes[][2];
extern __code const uint8_t diag_short_xdata_words[][2];
extern __code const uint8_t diag_adc_iram[];
extern __code const uint8_t diag_adc_xdata_bytes[][2];
extern __code const uint8_t diag_misfire_xdata_bytes[][2];

extern void append_iram_fields_to_serial(void);
extern void append_xdata_fields_to_serial(void);
extern void append_xdata_words_to_serial(void);
extern void append_serial_checksum(void);

#define START_DIAG_RESPONSE()                              \
    do {                                                   \
        const uint8_t saved_dpsel = DPSEL;                 \
        DPSEL = 7;                                         \
        serial_tx_buffer[0] = 0x80;                        \
        serial_tx_buffer[1] = serial_response_address;     \
        serial_tx_buffer[2] = 0x10;                        \
        serial_tx_buffer[4] = 0x61;                        \
        serial_tx_buffer[5] = stock_r4;                    \
        STOCK_DPTR = (uint16_t)serial_tx_buffer + 6;       \
        serial_tx_count = 6;                               \
        DPSEL = saved_dpsel;                               \
    } while (0)

#define APPEND_DIAG_FIELDS(table, append)                  \
    do {                                                   \
        const uint16_t address = (uint16_t)(table);        \
        stock_r3 = address >> 8;                           \
        stock_r4 = (uint8_t)address;                       \
        append();                                          \
    } while (0)

#define FINISH_DIAG_RESPONSE()                             \
    do {                                                   \
        const uint8_t saved_dpsel = DPSEL;                 \
        DPSEL = 7;                                         \
        serial_tx_buffer[3] = serial_tx_count - 4;         \
        DPSEL = saved_dpsel;                               \
        append_serial_checksum();                          \
    } while (0)

void build_full_diag_response(void)
{
    START_DIAG_RESPONSE();
    APPEND_DIAG_FIELDS(diag_full_iram, append_iram_fields_to_serial);
    APPEND_DIAG_FIELDS(diag_full_xdata_bytes, append_xdata_fields_to_serial);
    APPEND_DIAG_FIELDS(diag_full_xdata_words, append_xdata_words_to_serial);
    FINISH_DIAG_RESPONSE();
}

void build_short_diag_response(void)
{
    START_DIAG_RESPONSE();
    APPEND_DIAG_FIELDS(diag_short_iram, append_iram_fields_to_serial);
    APPEND_DIAG_FIELDS(diag_short_xdata_bytes, append_xdata_fields_to_serial);
    APPEND_DIAG_FIELDS(diag_short_xdata_words, append_xdata_words_to_serial);
    FINISH_DIAG_RESPONSE();
}

void build_adc_diag_response(void)
{
    START_DIAG_RESPONSE();
    APPEND_DIAG_FIELDS(diag_adc_iram, append_iram_fields_to_serial);
    APPEND_DIAG_FIELDS(diag_adc_xdata_bytes, append_xdata_fields_to_serial);
    FINISH_DIAG_RESPONSE();
}

void build_misfire_diag_response(void)
{
    START_DIAG_RESPONSE();
    APPEND_DIAG_FIELDS(diag_misfire_xdata_bytes, append_xdata_fields_to_serial);
    FINISH_DIAG_RESPONSE();
}
