#include <stdint.h>

static volatile __data __at (0x4e) uint8_t coolant_nearest_index;
static volatile __xdata __at (0xf878) uint8_t warmup_duration;

extern __code const uint8_t warmup_duration_by_temp[39];
extern void warmup_setup_continue(void);

void set_warmup_duration(void)
{
    warmup_duration = warmup_duration_by_temp[coolant_nearest_index];

    warmup_setup_continue();
}
