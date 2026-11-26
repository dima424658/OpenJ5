#include "eeprom_triplet.h"

enum { HISTORY_LENGTH = 8, COPY_COUNT = 3 };

static __sfr __at (0x8b) TL1;
static __sfr __at (0x8d) TH1;
static __sfr16 __at (0x8382) STOCK_DPTR;

static volatile __data __at (5) uint8_t stock_r5;
static volatile __data __at (6) uint8_t stock_r6;
static volatile __data __at (7) uint8_t stock_r7;
static volatile __data __at (0x7e) uint8_t rejected_copies;

static volatile __xdata __at (0xf4a6) uint8_t shutdown_samples[8];
static volatile __xdata __at (0xf813) uint8_t coolant_sample_sum[2];
static volatile __xdata __at (0xf81b) uint8_t ignition_supply_sum[2];
static volatile __xdata __at (0xf81d) uint8_t throttle_sample_sum[2];
static volatile __xdata __at (0xf942) uint8_t misfire_status;
static volatile __xdata __at (0xf4ae) uint8_t fault_history[HISTORY_LENGTH];
static volatile __xdata __at (0xfa43) uint8_t misfire_history[HISTORY_LENGTH];

extern void read_eeprom_page1_bytes(void);
extern void write_eeprom_page1_bytes(void);
extern void write_startup_fault_history(void);
extern void recover_startup_fault_history(void);
extern void recover_startup_misfire_history(void);
extern void write_eeprom_shutdown_block(void);
extern void restore_misfire_thresholds(void);

#define MIX_SAMPLE_PAIR(slot, samples)                                   \
    do {                                                                 \
        stock_r0 = (samples)[0];                                         \
        stock_r1 = (samples)[1];                                         \
        shutdown_samples[(slot)] ^= stock_r0;                            \
        shutdown_samples[(slot) + 1] ^= stock_r1;                        \
    } while (0)

/* When several copies pass the checksum, the last accepted copy wins. */
#define SCAN_HISTORY(read, offset, history)                              \
    do {                                                                 \
        rejected_copies = 0;                                             \
        stock_r7 = 0;                                                    \
        stock_r6 = COPY_COUNT;                                           \
        do {                                                             \
            EEPROM_IO(read, (offset) + HISTORY_LENGTH * stock_r7, HISTORY_LENGTH);\
            stock_r0 = EEPROM_SCRATCH_ADDRESS;                           \
            stock_r1 = HISTORY_LENGTH;                                   \
            stock_r4 = 0;                                                \
            do {                                                         \
                stock_r4 += *(volatile __idata uint8_t *)stock_r0;       \
                ++stock_r0;                                              \
            } while (--stock_r1);                                        \
            if (stock_r4 != stock_r5) {                                  \
                ++rejected_copies;                                       \
            } else {                                                     \
                stock_r0 = EEPROM_SCRATCH_ADDRESS;                       \
                stock_r1 = HISTORY_LENGTH;                               \
                STOCK_DPTR = (uint16_t)(history);                        \
                do {                                                     \
                    *(volatile __xdata uint8_t *)STOCK_DPTR =            \
                        *(volatile __idata uint8_t *)stock_r0;           \
                    ++stock_r0;                                          \
                    ++STOCK_DPTR;                                        \
                } while (--stock_r1);                                    \
            }                                                            \
            ++stock_r7;                                                  \
        } while (--stock_r6);                                            \
    } while (0)

void mix_shutdown_timer_samples(void)
{
    shutdown_samples[0] ^= TH1;
    shutdown_samples[1] ^= TL1;
    MIX_SAMPLE_PAIR(2, coolant_sample_sum);
    MIX_SAMPLE_PAIR(4, ignition_supply_sum);
    MIX_SAMPLE_PAIR(6, throttle_sample_sum);
}

void read_startup_fault_history_sum(void)
{
    READ_TRIPLET(read_eeprom_bytes, write_eeprom_bytes, 0x98);
    stock_r5 = scratch[0];
}

void read_misfire_history_sum(void)
{
    READ_TRIPLET(read_eeprom_page1_bytes, write_eeprom_page1_bytes, 0x18);
    stock_r5 = scratch[0];
}

void read_startup_misfire_status(void)
{
    READ_TRIPLET(read_eeprom_page1_bytes, write_eeprom_page1_bytes, 0x1c);
    ACC = scratch[0];
    if (ACC >= 0x10) {
        ACC = 0;
    }
    misfire_status = ACC;
}

void read_startup_fault_history(void)
{
    read_startup_fault_history_sum();
    SCAN_HISTORY(read_eeprom_bytes, 0x80, fault_history);
    if (rejected_copies == COPY_COUNT) {
        recover_startup_fault_history();
    } else if (rejected_copies != 0) {
        STOCK_DPTR = (uint16_t)fault_history;
        write_startup_fault_history();
    }
}

void restore_startup_eeprom_bd12(void)
{
    restore_misfire_thresholds();
    read_startup_misfire_status();
    read_misfire_history_sum();
    SCAN_HISTORY(read_eeprom_page1_bytes, 0x00, misfire_history);
    if (rejected_copies == COPY_COUNT) {
        recover_startup_misfire_history();
    } else if (rejected_copies != 0) {
        STOCK_DPTR = (uint16_t)misfire_history;
        write_eeprom_shutdown_block();
    }

    if (misfire_history[1] == 0xff) {
        misfire_history[0] = 0;
        misfire_history[2] = 0;
        misfire_history[4] = 0;
        misfire_history[6] = 0;
        misfire_history[1] = 0x80;
        misfire_history[3] = 0x80;
        misfire_history[5] = 0x80;
        misfire_history[7] = 0x80;
    }
}
