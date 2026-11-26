#include "stock_registers.h"

static __sfr __at (0xe9) mdu_0;
static __sfr __at (0xea) mdu_1;
static __sfr __at (0xed) mdu_4;
static __sfr __at (0xee) mdu_5;

static __sbit __at (0x00) monitor_command_active;
static __sbit __at (0x02) monitor_result_ready;
static __sbit __at (0x03) monitor_checksum_failed;

static volatile __data __at (0x42) uint16_t monitor_mdu_test_result;

extern void resume_service_monitor(void);

enum {
    MDU_TEST_DIVIDEND = 10000,
    MDU_TEST_DIVISOR = 300,
    MDU_TEST_QUOTIENT = MDU_TEST_DIVIDEND / MDU_TEST_DIVISOR,
    MDU_TEST_REMAINDER = MDU_TEST_DIVIDEND % MDU_TEST_DIVISOR,
    MDU_TEST_FAILURE = 0x1234,
};

#define NOP() __asm nop __endasm
#define WAIT_FOUR_CYCLES() do { NOP(); NOP(); NOP(); NOP(); } while (0)

void test_monitor_mdu_divide(void)
{
    mdu_0 = (uint8_t)MDU_TEST_DIVIDEND;
    mdu_1 = MDU_TEST_DIVIDEND >> 8;
    mdu_4 = (uint8_t)MDU_TEST_DIVISOR;
    mdu_5 = MDU_TEST_DIVISOR >> 8;
    WAIT_FOUR_CYCLES();

    stock_r0 = mdu_0;
    stock_r1 = mdu_1;
    stock_r2 = mdu_4;
    stock_r3 = mdu_5;
    if (stock_r01 == MDU_TEST_QUOTIENT && stock_r23 == MDU_TEST_REMAINDER) {
        stock_r01 = 0;
    } else {
        stock_r01 = MDU_TEST_FAILURE;
    }
    monitor_mdu_test_result = stock_r01;
    ACC = stock_r0 | stock_r1;
    if (ACC != 0) {
        monitor_checksum_failed = 1;
    }
    monitor_command_active = 0;
    monitor_result_ready = 1;
    resume_service_monitor();
}

void monitor_delay_acc(void)
{
    do {
        WAIT_FOUR_CYCLES();
    } while (--ACC);
}
