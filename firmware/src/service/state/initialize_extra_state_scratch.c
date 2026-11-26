#include "stock_registers.h"

static __sfr16 __at (0x8382) STOCK_DPTR;
static volatile __data __at (0x21) uint8_t state_history[4];
static volatile __xdata __at (0xf4ae) uint8_t retained_seed[8];
static volatile __xdata __at (0xf4b2) uint8_t retained_tail[4];
static volatile __xdata __at (0xf4a6) uint8_t retained_record[6];

extern const __code uint8_t extra_state_seed[8];
extern const __code uint8_t extra_state_seed_tail[6];

#define SCRATCH_BYTE (*(volatile __idata uint8_t *)stock_r0)
#define READ_XDATA (*(volatile __xdata uint8_t *)STOCK_DPTR)
#define READ_CODE (*(const __code uint8_t *)STOCK_DPTR)

#define COPY_TO_SCRATCH(address, count, source) \
    do {                                        \
        STOCK_DPTR = (uint16_t)(address);       \
        stock_r2 = (count);                     \
        do {                                    \
            ACC = (source);                     \
            SCRATCH_BYTE = ACC;                 \
            ++stock_r0;                         \
            ++STOCK_DPTR;                       \
        } while (--stock_r2);                   \
    } while (0)

void initialize_extra_state_scratch(void)
{
    stock_r0 = 0x50;
    COPY_TO_SCRATCH(retained_seed, sizeof(retained_seed), READ_XDATA);
    COPY_TO_SCRATCH(extra_state_seed, sizeof(extra_state_seed), READ_CODE);
    COPY_TO_SCRATCH(extra_state_seed_tail, 4, READ_CODE);
    COPY_TO_SCRATCH(retained_tail, sizeof(retained_tail), READ_XDATA);
    COPY_TO_SCRATCH(extra_state_seed, sizeof(extra_state_seed), READ_CODE);

    STOCK_DPTR = (uint16_t)extra_state_seed_tail;
    ACC = READ_CODE;
    SCRATCH_BYTE = ACC;

    stock_r0 = 0x25;
    COPY_TO_SCRATCH(retained_record, sizeof(retained_record), READ_XDATA);

    stock_r0 = 0x50;
    state_history[3] = SCRATCH_BYTE;
    ++stock_r0;
    state_history[2] = SCRATCH_BYTE;
    ++stock_r0;
    state_history[1] = SCRATCH_BYTE;
    ++stock_r0;
    state_history[0] = SCRATCH_BYTE;
    ++stock_r0;
    ACC = stock_r0;
    stock_r2 = ACC;
    stock_r1 = 0x25;
}
