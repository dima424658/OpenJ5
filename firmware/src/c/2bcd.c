#include <stdint.h>

static volatile __data __at (0x54) uint8_t rpm_code;
static volatile __data __at (0x57) uint8_t rpm_index_q4;
static volatile __data __at (0x58) uint8_t rpm_nearest_half_bin;
static volatile __data __at (0x59) uint8_t rpm_nearest_bin;

extern __code const uint8_t rpm_quantization_q4[256];
extern void rpm_update_continued(void);

/* The incoming engine speed code is already clamped to one byte. */
void update_rpm_axis(void)
{
    const uint8_t index = rpm_quantization_q4[rpm_code];
    rpm_index_q4 = index;
    rpm_nearest_half_bin = (uint8_t)(index + 4) >> 3;
    rpm_nearest_bin = (uint8_t)(rpm_index_q4 + 8) >> 4;

    rpm_update_continued();
}
