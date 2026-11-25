#include "stock_registers.h"

enum {
    CHECKSUM_BYTES_PER_STEP = 8,
    CALIBRATION_LOW_START = 0x6000,
    CALIBRATION_LOW_ACTIVE = 0x01,
    CALIBRATION_HIGH_ACTIVE = 0x02,
    CALIBRATION_LOW_COMPLETE = 0x10,
    CALIBRATION_HIGH_COMPLETE = 0x20,
};

#define CALIBRATION_HIGH_START UINT16_C(0x8000)
#define CALIBRATION_END UINT16_C(0xa000)

static __sbit __at (0x17) rom_checksum_failed;
static __sbit __at (0x47) flash_checksum_started;
static __sbit __at (0xe0) ACC_BIT0;
static __sbit __at (0xe1) ACC_BIT1;
static __sbit __at (0xe4) ACC_BIT4;
static __sbit __at (0xe5) ACC_BIT5;

static volatile __xdata __at (0xf8f8) uint16_t flash_checksum_result;
static volatile __xdata __at (0xf8fa) uint16_t flash_checksum_partial;
static volatile __xdata __at (0xf8fc) uint16_t flash_checksum_next;
static volatile __xdata __at (0xf9bf) uint8_t calibration_checksum_state;
static volatile __xdata __at (0xf9c0) uint16_t calibration_low_partial;
static volatile __xdata __at (0xf9c2) uint16_t calibration_high_partial;
static volatile __xdata __at (0xf9c4) uint16_t calibration_checksum_next;
static volatile __xdata __at (0xf9c6) uint8_t flash_checksum_complete;

extern volatile __code const uint16_t flash_checksum_expected;
extern volatile __code const uint16_t calibration_low_expected;
extern volatile __code const uint16_t calibration_high_expected;

extern void subtract_u16_register_pairs(void);
extern void run_service_mode_53b1(void);
void update_calibration_low_checksum(void);
void update_calibration_high_sum(void);

// Each scan has its own in-place eight-byte loop in the linked image.
#define SUM_ROM_CHUNK() do { \
    for (uint8_t offset = 0; offset < CHECKSUM_BYTES_PER_STEP; ++offset) { \
        stock_r01 += *(const __code uint8_t *)stock_r23; \
        ++stock_r23; \
    } \
} while (0)

void continue_service_normal(void)
{
    if (!flash_checksum_complete) {
        if (!flash_checksum_started) {
            flash_checksum_partial = 0;
            flash_checksum_next = 0;
            flash_checksum_started = 1;
        }
        stock_r01 = flash_checksum_partial;
        stock_r23 = flash_checksum_next;
        SUM_ROM_CHUNK();
        flash_checksum_next = stock_r23;
        flash_checksum_partial = stock_r01;

        if (stock_r23 == 0) {
            flash_checksum_complete = 1;
            flash_checksum_started = 0;
            flash_checksum_result = stock_r01;
            if (!rom_checksum_failed) {
                stock_r23 = flash_checksum_expected;
                subtract_u16_register_pairs();
                if (stock_r01) {
                    rom_checksum_failed = 1;
                } else {
                    rom_checksum_failed = 0;
                }
            }
        }
    }
    update_calibration_low_checksum();
}

void update_calibration_low_checksum(void)
{
    ACC = calibration_checksum_state;
    if (ACC == 0) {
        calibration_checksum_state = CALIBRATION_LOW_ACTIVE;
        calibration_low_partial = 0;
        calibration_high_partial = 0;
        calibration_checksum_next = CALIBRATION_LOW_START;
    } else if (ACC_BIT4 || !ACC_BIT0) {
        update_calibration_high_sum();
        return;
    }

    stock_r01 = calibration_low_partial;
    stock_r23 = calibration_checksum_next;
    SUM_ROM_CHUNK();
    calibration_checksum_next = stock_r23;
    calibration_low_partial = stock_r01;

    if (stock_r23 == CALIBRATION_HIGH_START) {
        calibration_checksum_state = CALIBRATION_LOW_COMPLETE;
        stock_r23 = calibration_low_expected;
        subtract_u16_register_pairs();
        if (stock_r01) {
            rom_checksum_failed = 1;
        } else {
            rom_checksum_failed = 0;
        }
    }
    run_service_mode_53b1();
}

void update_calibration_high_sum(void)
{
    // The dispatcher passes the checksum state in ACC.
    if (!ACC_BIT5) {
        if (!ACC_BIT1) {
            calibration_checksum_state = ACC | CALIBRATION_HIGH_ACTIVE;
            calibration_checksum_next = CALIBRATION_HIGH_START;
        }

        stock_r01 = calibration_high_partial;
        stock_r23 = calibration_checksum_next;
        SUM_ROM_CHUNK();
        calibration_checksum_next = stock_r23;
        calibration_high_partial = stock_r01;

        if (stock_r23 == CALIBRATION_END) {
            calibration_checksum_state |= CALIBRATION_HIGH_COMPLETE;
            if (!rom_checksum_failed) {
                stock_r23 = calibration_high_expected;
                subtract_u16_register_pairs();
                if (stock_r01) {
                    rom_checksum_failed = 1;
                } else {
                    rom_checksum_failed = 0;
                }
            }
        }
    }
    run_service_mode_53b1();
}
