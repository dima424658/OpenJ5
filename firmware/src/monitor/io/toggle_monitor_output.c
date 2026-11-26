#include "stock_registers.h"

enum { MONITOR_TOGGLE_DELAY = 1000 };

static __sbit __at (0xed) monitor_output_ed;

extern void poll_monitor_exit(void);

void toggle_monitor_output(void)
{
    for (;;) {
        stock_r01 = 0;
        do {
            ++stock_r01;
        } while (stock_r01 != MONITOR_TOGGLE_DELAY);
        poll_monitor_exit();
        monitor_output_ed = !monitor_output_ed;
    }
}
