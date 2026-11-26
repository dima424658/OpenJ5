#include "stock_registers.h"

enum {
    TRANSFER_SOURCE_DPTR = 3,
    TRANSFER_DESTINATION_DPTR = 4,
    SERIAL_TX_DPTR = 7,
};

static __sfr __at (0x82) DPL;
static __sfr __at (0x83) DPH;
static __sfr __at (0x92) DPSEL;
static __sfr16 __at (0x8382) STOCK_DPTR;

static volatile __data __at (4) uint8_t stock_r4;
static volatile __data __at (0x51) uint8_t serial_tx_count;

/* The current DPTR bank determines which cursor the operation advances. */
#define READ_CODE_BYTE() (*(const __code uint8_t *)STOCK_DPTR)
#define READ_XDATA_BYTE() (*(volatile __xdata uint8_t *)STOCK_DPTR)
#define WRITE_XDATA_BYTE(value) (*(volatile __xdata uint8_t *)STOCK_DPTR = (value))

#define LOAD_SOURCE_POINTER()\
    do {                     \
        DPH = stock_r3;      \
        DPL = stock_r4;      \
    } while (0)

#define APPEND_TX_BYTE()     \
    do {                     \
        DPSEL = SERIAL_TX_DPTR;\
        WRITE_XDATA_BYTE(ACC);\
        ++STOCK_DPTR;        \
        ++serial_tx_count;   \
    } while (0)

void append_xdata_string_to_serial(void)
{
    LOAD_SOURCE_POINTER();
    while ((ACC = READ_XDATA_BYTE()) != 0) {
        uint8_t saved_dpsel = DPSEL;

        APPEND_TX_BYTE();
        DPSEL = saved_dpsel;
        ++STOCK_DPTR;
    }
}

void append_code_string_to_serial(void)
{
    LOAD_SOURCE_POINTER();
    while ((ACC = READ_CODE_BYTE()) != 0) {
        uint8_t saved_dpsel = DPSEL;

        APPEND_TX_BYTE();
        DPSEL = saved_dpsel;
        ++STOCK_DPTR;
    }
}

void copy_xdata_bytes(void)
{
    /* An initial count of zero wraps and copies 256 bytes. */
    do {
        DPSEL = TRANSFER_SOURCE_DPTR;
        ACC = READ_XDATA_BYTE();
        ++STOCK_DPTR;
        DPSEL = TRANSFER_DESTINATION_DPTR;
        WRITE_XDATA_BYTE(ACC);
        ++STOCK_DPTR;
    } while (--stock_r2);
}

void copy_code_bytes_to_xdata(void)
{
    do {
        DPSEL = TRANSFER_SOURCE_DPTR;
        ACC = READ_CODE_BYTE();
        ++STOCK_DPTR;
        DPSEL = TRANSFER_DESTINATION_DPTR;
        WRITE_XDATA_BYTE(ACC);
        ++STOCK_DPTR;
    } while (--stock_r2);
}

void append_code_bytes_to_serial(void)
{
    LOAD_SOURCE_POINTER();
    do {
        uint8_t saved_dpsel;

        ACC = READ_CODE_BYTE();
        ++STOCK_DPTR;
        saved_dpsel = DPSEL;
        APPEND_TX_BYTE();
        DPSEL = saved_dpsel;
    } while (--stock_r2);
}

void copy_code_string_to_xdata(void)
{
    for (;;) {
        DPSEL = TRANSFER_SOURCE_DPTR;
        ACC = READ_CODE_BYTE();
        if (ACC == 0) {
            break;
        }
        ++STOCK_DPTR;
        DPSEL = TRANSFER_DESTINATION_DPTR;
        WRITE_XDATA_BYTE(ACC);
        ++STOCK_DPTR;
    }
    DPSEL = TRANSFER_DESTINATION_DPTR;
    WRITE_XDATA_BYTE(ACC);
}

void copy_xdata_string(void)
{
    for (;;) {
        DPSEL = TRANSFER_SOURCE_DPTR;
        ACC = READ_XDATA_BYTE();
        if (ACC == 0) {
            break;
        }
        ++STOCK_DPTR;
        DPSEL = TRANSFER_DESTINATION_DPTR;
        WRITE_XDATA_BYTE(ACC);
        ++STOCK_DPTR;
    }
    DPSEL = TRANSFER_DESTINATION_DPTR;
    WRITE_XDATA_BYTE(ACC);
}

void append_iram_fields_to_serial(void)
{
    LOAD_SOURCE_POINTER();
    while ((ACC = READ_CODE_BYTE()) != 0) {
        uint8_t saved_dpsel;

        stock_r0 = ACC;
        ACC = *(volatile __idata uint8_t *)stock_r0;
        ++STOCK_DPTR;
        saved_dpsel = DPSEL;
        APPEND_TX_BYTE();
        DPSEL = saved_dpsel;
    }
}

void append_xdata_words_to_serial(void)
{
    LOAD_SOURCE_POINTER();
    for (;;) {
        uint8_t saved_dpsel;

        stock_r3 = 2;
        ACC = READ_CODE_BYTE();
        if (ACC == 0) {
            break;
        }
        stock_r4 = ACC;
        ++STOCK_DPTR;
        ACC = READ_CODE_BYTE();
        ++STOCK_DPTR;
        saved_dpsel = DPSEL;

        DPSEL = TRANSFER_SOURCE_DPTR;
        DPH = stock_r4;
        DPL = ACC;
        do {
            ACC = READ_XDATA_BYTE();
            ++STOCK_DPTR;
            APPEND_TX_BYTE();
            DPSEL = TRANSFER_SOURCE_DPTR;
        } while (--stock_r3);
        DPSEL = saved_dpsel;
    }
}

void append_xdata_fields_to_serial(void)
{
    LOAD_SOURCE_POINTER();
    while ((ACC = READ_CODE_BYTE()) != 0) {
        uint8_t saved_dpsel;

        stock_r4 = ACC;
        ++STOCK_DPTR;
        ACC = READ_CODE_BYTE();
        ++STOCK_DPTR;
        saved_dpsel = DPSEL;

        DPSEL = TRANSFER_SOURCE_DPTR;
        DPH = stock_r4;
        DPL = ACC;
        ACC = READ_XDATA_BYTE();
        ++STOCK_DPTR;
        APPEND_TX_BYTE();
        DPSEL = saved_dpsel;
    }
}
