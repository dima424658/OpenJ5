#include "stock_registers.h"

enum {
    FAULT_COUNT = 64,
    DIAGNOSTIC_RECORD_SIZE = 3,
    HISTORY_REPORT_MASK = 0xf0,
};

static __sfr __at (0x92) DPSEL;
static __sfr __at (0xf0) B;

static volatile __idata __at (0x93) uint8_t diagnostic_record_count;
static volatile __xdata __at (0xf512) uint8_t fault_history[FAULT_COUNT];
static volatile __xdata __at (0xf400) uint8_t diagnostic_records[FAULT_COUNT * DIAGNOSTIC_RECORD_SIZE + 1];

extern __code const uint8_t diagnostic_fault_codes[FAULT_COUNT][2];

void collect_fault_status_records(void)
{
    diagnostic_record_count = 0;
    diagnostic_records[0] = 0;
    uint8_t record_offset = 0;
    // The stock routine keeps the history slot index in R1.
    for (stock_r1 = 0; stock_r1 < FAULT_COUNT; ++stock_r1) {
        const uint8_t status = fault_history[stock_r1] & HISTORY_REPORT_MASK;
        if (status != 0) {
            const __code uint8_t *code = diagnostic_fault_codes[stock_r1];
            const uint8_t high = code[0];
            B = code[1];
            diagnostic_records[record_offset] = high;
            diagnostic_records[record_offset + 1] = B;
            diagnostic_records[record_offset + 2] = status;
            // The next record replaces this terminator.
            record_offset += DIAGNOSTIC_RECORD_SIZE;
            diagnostic_records[record_offset] = 0;
            ++diagnostic_record_count;
        }
    }
}
