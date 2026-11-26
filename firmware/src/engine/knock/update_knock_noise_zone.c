#include <stdint.h>

enum {
    Q4_HALF_STEP = 8,
    Q4_INTEGER_MASK = 0xf0,
    NOISE_ZONE_NUMBER_MASK = 0x0f,
};

static volatile __data __at (0x52) uint8_t throttle_position;
static volatile __data __at (0x59) uint8_t rpm_nearest_bin;
static volatile __xdata __at (0xf979) uint8_t throttle_nearest_bin;
static volatile __xdata __at (0xf933) uint8_t knock_noise_zone;
static volatile __xdata __at (0xf932) uint8_t knock_noise_zone_number;
static __sbit __at (0x37) noise_adaptation_active;
static __sbit __at (0x62) inhibit_noise_adaptation_62;

static __sbit __at (0xd7) CY;
static __sfr __at (0xe0) ACC;
static __sfr __at (0xf0) B;
static __sbit __at (0xf4) NOISE_ADAPTATION_ZONE;

extern __code const uint8_t throttle_quantization_q4[101];
extern __code const uint8_t knock_noise_zone_flags[256];
extern void knock_zone_update_continued(void);

/* B holds the rounded row offset, then the flags from the selected cell. */
void update_knock_noise_zone(void)
{
    const uint8_t throttle = throttle_quantization_q4[throttle_position];
    B = (uint8_t)(throttle + Q4_HALF_STEP) & Q4_INTEGER_MASK;
    throttle_nearest_bin = B >> 4;

    const uint8_t zone_index = B + rpm_nearest_bin;
    ACC = knock_noise_zone_flags[zone_index];
    B = ACC;
    CY = !inhibit_noise_adaptation_62;
    noise_adaptation_active = CY & NOISE_ADAPTATION_ZONE;
    knock_noise_zone = ACC;
    knock_noise_zone_number = ACC & NOISE_ZONE_NUMBER_MASK;

    knock_zone_update_continued();
}
