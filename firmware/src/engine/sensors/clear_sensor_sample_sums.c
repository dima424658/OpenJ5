#include <stdint.h>

static volatile __xdata __at (0xf813) uint8_t sensor_sample_sums[16];

void clear_sensor_sample_sums(void)
{
    volatile __xdata uint8_t *next = sensor_sample_sums;
    uint8_t remaining = sizeof(sensor_sample_sums);
    do {
        *next++ = 0;
    } while (--remaining != 0);
}
