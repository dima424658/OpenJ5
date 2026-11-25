#include "stock_registers.h"

enum { MISFIRE_WINDOW_RELOAD = 0x37 };

static __sfr __at (0xf0) B;
static __sbit __at (0xe0) ACC_BIT0;
static volatile __data __at (0x43) uint8_t cylinder_slot;

static volatile __xdata __at (0xf9f5) uint8_t misfire_event_flags;
static volatile __xdata __at (0xfa09) uint8_t misfire_filtered_flags;
static volatile __xdata __at (0xfa6b) uint8_t misfire_enabled_flags;
static volatile __xdata __at (0xf8cf) uint8_t active_misfire_flags;
static volatile __xdata __at (0xf8ce) uint8_t misfire_event_weight;
static volatile __xdata __at (0xf8c6) uint16_t live_misfire_counts[4];
static volatile __xdata __at (0xfa5d) uint8_t misfire_fuel_cut_events;
static volatile __xdata __at (0xfa62) uint8_t misfire_window_tick;
static volatile __xdata __at (0xfa63) uint16_t saved_misfire_counts[4];
static volatile __xdata __at (0xfa6c) uint8_t misfire_window_flags;
static volatile __xdata __at (0xfa6e) uint16_t misfire_window_total;

extern void continue_crank_event(void);

void combine_misfire_event_flags(void);
void advance_misfire_window(void);
void publish_misfire_window(void);
void store_misfire_window_tick(void);

void skip_misfire_period_update(void)
{
    combine_misfire_event_flags();
}

void skip_misfire_period_scaling(void)
{
    ACC = 0;
    misfire_fuel_cut_events = ACC;
    combine_misfire_event_flags();
}

// The live counters are stored in reverse slot order.
#define COUNT_MISFIRE_SLOT(slot)                     \
    do {                                             \
        if (active_misfire_flags & (1u << (slot))) { \
            B = misfire_event_weight;                \
            live_misfire_counts[3 - (slot)] += B;    \
        }                                            \
    } while (0)

void combine_misfire_event_flags(void)
{
    B = misfire_event_flags | misfire_filtered_flags;
    ACC = misfire_enabled_flags & B;
    active_misfire_flags = ACC;
    if (ACC != 0) {
        switch (cylinder_slot) {
        case 0:
            COUNT_MISFIRE_SLOT(0);
            break;
        case 1:
            COUNT_MISFIRE_SLOT(1);
            break;
        case 2:
            COUNT_MISFIRE_SLOT(2);
            break;
        case 3:
            COUNT_MISFIRE_SLOT(3);
            break;
        }
    }
    advance_misfire_window();
}

void advance_misfire_window(void)
{
    ACC = cylinder_slot;
    if (ACC_BIT0) {
        continue_crank_event();
        return;
    }
    ACC = misfire_window_tick + 1;
    if (ACC != 0) {
        store_misfire_window_tick();
        return;
    }
    publish_misfire_window();
}

#define SAVE_SLOT_COUNT(slot)                                   \
    do {                                                        \
        const uint16_t count = live_misfire_counts[3 - (slot)]; \
        saved_misfire_counts[(slot)] = count;                   \
        stock_r01 += count;                                     \
    } while (0)

void publish_misfire_window(void)
{
    stock_r01 = live_misfire_counts[3];
    saved_misfire_counts[0] = stock_r01;
    SAVE_SLOT_COUNT(1);
    SAVE_SLOT_COUNT(2);
    const uint16_t last = live_misfire_counts[0];
    saved_misfire_counts[3] = last;

    // Only a carry from the final addition saturates the published total.
    const uint32_t total = (uint32_t)stock_r01 + last;
    stock_r01 = total > UINT16_MAX ? UINT16_MAX : (uint16_t)total;
    misfire_window_total = stock_r01;

    live_misfire_counts[3] = 0;
    live_misfire_counts[2] = 0;
    live_misfire_counts[1] = 0;
    live_misfire_counts[0] = 0;
    misfire_window_flags |= 1;
    ACC = MISFIRE_WINDOW_RELOAD;
    store_misfire_window_tick();
}

void store_misfire_window_tick(void)
{
    misfire_window_tick = ACC;
    continue_crank_event();
}
