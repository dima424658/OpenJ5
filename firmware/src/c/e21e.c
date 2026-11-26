#include "stock_registers.h"

static __sfr __at (0xf0) B;
static __sbit __at (0xaf) EA;

static volatile __data __at (0x20) uint8_t monitor_flags;
static volatile __data __at (0x30) uint8_t monitor_flags_snapshot;
static volatile __data __at (0x41) uint8_t monitor_request_offset;
static volatile __data __at (0x7c) uint8_t monitor_latched_events[2];
static volatile __idata __at (0x80) uint8_t monitor_event_report[2];

extern void read_monitor_aux_status(void);

/* The first event in either source sets both low bits. The next three
 * events occupy one bit per source in each of the remaining pairs.
 */
#define PACK_EVENT_NIBBLE(index, offset)                                    \
    do {                                                                    \
        const uint8_t first = (ACC >> (offset)) & 0x0f;                      \
        const uint8_t second = (B >> (offset)) & 0x0f;                       \
        stock_r2 = 0;                                                       \
        if (first & 1) { stock_r2 |= 0x03; }                                \
        if (second & 1) { stock_r2 |= 0x03; }                               \
        if (first & 2) { stock_r2 |= 0x08; }                                \
        if (second & 2) { stock_r2 |= 0x04; }                               \
        if (first & 4) { stock_r2 |= 0x20; }                                \
        if (second & 4) { stock_r2 |= 0x10; }                               \
        if (first & 8) { stock_r2 |= 0x80; }                                \
        if (second & 8) { stock_r2 |= 0x40; }                               \
        monitor_event_report[index] = stock_r2;                             \
    } while (0)

void resume_service_monitor(void)
{
    monitor_flags_snapshot = monitor_flags;
    monitor_request_offset = 0;

    const uint8_t irq_enabled = EA;
    EA = 0;
    ACC = monitor_latched_events[0];
    monitor_latched_events[0] = 0;
    B = monitor_latched_events[1];
    monitor_latched_events[1] = 0;
    EA = irq_enabled;

    PACK_EVENT_NIBBLE(0, 0);
    PACK_EVENT_NIBBLE(1, 4);

    stock_r0 = (uint8_t)((uint16_t)monitor_event_report + 2);
    stock_r2 = 0;
    read_monitor_aux_status();
}
