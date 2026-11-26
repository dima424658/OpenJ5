#include "eeprom_triplet.h"

enum {
    IMMOBILIZER_RECORD_LENGTH = 4,
    FIRST_EEPROM_RECORD = 0x08,
    SECOND_EEPROM_RECORD = 0x0c,
    THIRD_EEPROM_RECORD = 0x2c,
    FIRST_READ_FAILED = 0x01,
    SECOND_READ_FAILED = 0x02,
    THIRD_READ_FAILED = 0x04,
    READ_FAILURES = 0x0f,
    SECOND_RECORD_DIFFERS = 0x10,
    THIRD_RECORD_DIFFERS = 0x20,
};

static __sfr16 __at (0x8382) STOCK_DPTR;
static __sbit __at (0xe4) ACC_BIT4;
static __sbit __at (0xe5) ACC_BIT5;
static volatile __data __at (7) uint8_t stock_r7;

static volatile __xdata __at (0xf4bd) uint8_t selected_record[IMMOBILIZER_RECORD_LENGTH];
static volatile __xdata __at (0xf4c1) uint8_t second_record[IMMOBILIZER_RECORD_LENGTH];
static volatile __xdata __at (0xf4c5) uint8_t immobilizer_status;

extern void initialize_immobilizer_records(void);
extern void copy_eeprom_record_to_xdata(void);
extern void copy_xdata_to_eeprom_buffer(void);
extern void compare_eeprom_record_byte(void);

static void repair_immobilizer_records(void);

#define SELECT_EEPROM_RECORD(offset)           \
    do {                                        \
        stock_r4 = (offset);                    \
        stock_r1 = IMMOBILIZER_RECORD_LENGTH;   \
        stock_r0 = EEPROM_SCRATCH_ADDRESS;      \
    } while (0)

void restore_immobilizer_records(void)
{
    initialize_immobilizer_records();

    SELECT_EEPROM_RECORD(FIRST_EEPROM_RECORD);
    read_eeprom_bytes();
    stock_r7 = 0;
    if (CARRY) {
        eeprom_io_failed = 1;
        stock_r7 = FIRST_READ_FAILED;
    } else {
        STOCK_DPTR = (uint16_t)selected_record;
        copy_eeprom_record_to_xdata();
    }

    SELECT_EEPROM_RECORD(SECOND_EEPROM_RECORD);
    read_eeprom_bytes();
    /* The stock routine copies the second buffer before checking CY. */
    STOCK_DPTR = (uint16_t)second_record;
    copy_eeprom_record_to_xdata();
    if (CARRY) {
        eeprom_io_failed = 1;
        stock_r7 |= SECOND_READ_FAILED;
    } else if (stock_r7 != 0) {
        STOCK_DPTR = (uint16_t)selected_record;
        copy_eeprom_record_to_xdata();
    } else {
        STOCK_DPTR = (uint16_t)selected_record;
        compare_eeprom_record_byte();
        if (CARRY) {
            stock_r7 |= SECOND_RECORD_DIFFERS;
        }
    }

    SELECT_EEPROM_RECORD(THIRD_EEPROM_RECORD);
    read_eeprom_bytes();
    if (CARRY) {
        eeprom_io_failed = 1;
        stock_r7 |= THIRD_READ_FAILED;
    } else if (stock_r7 & READ_FAILURES) {
        STOCK_DPTR = (uint16_t)selected_record;
        copy_eeprom_record_to_xdata();
        goto finish_read;
    }
    STOCK_DPTR = (uint16_t)selected_record;
    compare_eeprom_record_byte();
    if (CARRY) {
        stock_r7 |= THIRD_RECORD_DIFFERS;
    }

finish_read:
    if (stock_r7 != 0) {
        repair_immobilizer_records();
    }
    ACC = selected_record[3];
    immobilizer_status = ACC | 0xfe;
}

static void repair_immobilizer_records(void)
{
    if (stock_r7 & READ_FAILURES) {
        goto failed;
    }

    ACC = stock_r7;
    if (ACC_BIT4 && ACC_BIT5) {
        STOCK_DPTR = (uint16_t)second_record;
        copy_xdata_to_eeprom_buffer();
        STOCK_DPTR = (uint16_t)selected_record;
        compare_eeprom_record_byte();
        if (CARRY) {
            goto failed;
        }
        SELECT_EEPROM_RECORD(FIRST_EEPROM_RECORD);
        write_eeprom_bytes();
        if (!CARRY) {
            return;
        }
        goto failed;
    }

    ACC = stock_r7;
    if (ACC_BIT4) {
        STOCK_DPTR = (uint16_t)selected_record;
        copy_xdata_to_eeprom_buffer();
        SELECT_EEPROM_RECORD(SECOND_EEPROM_RECORD);
        write_eeprom_bytes();
        if (!CARRY) {
            return;
        }
        goto failed;
    }

    if (ACC_BIT5) {
        STOCK_DPTR = (uint16_t)selected_record;
        copy_xdata_to_eeprom_buffer();
        SELECT_EEPROM_RECORD(THIRD_EEPROM_RECORD);
        write_eeprom_bytes();
        if (CARRY) {
            goto failed;
        }
    }
    return;

failed:
    eeprom_io_failed = 1;
}
