#include <stdint.h>

static __sbit __at (0x28) knock_low_noise_fault;

static volatile __data __at (0x58) uint8_t rpm_half_step_index;
static volatile __xdata __at (0xf96e) uint8_t cylinder_noise[4];
static volatile __xdata __at (0xf980) uint8_t combined_knock_noise;
static volatile __xdata __at (0xf985) uint8_t low_knock_noise_counter;

extern __code const uint8_t knock_minimum_noise_by_rpm[32];

void combine_knock_corrections(void)
{
    const uint8_t first = cylinder_noise[0];
    const uint8_t second = cylinder_noise[1];
    const uint8_t third = cylinder_noise[2];
    const uint8_t fourth = cylinder_noise[3];

    // The stock loop halves the first pair, adds the rounded third term,
    // halves that subtotal, then adds the rounded fourth term.
    const uint16_t first_pair = ((uint16_t)first + second) >> 1;
    const uint16_t first_three = (first_pair + (((uint16_t)third + 1) >> 1)) >> 1;
    const uint8_t noise = first_three + (((uint16_t)fourth + 2) >> 2);
    combined_knock_noise = noise;

    const uint8_t minimum = knock_minimum_noise_by_rpm[rpm_half_step_index];
    if (noise <= minimum) {
        const uint8_t next = low_knock_noise_counter + 1;
        if (next == 0) {
            knock_low_noise_fault = 1;
        } else {
            low_knock_noise_counter = next;
        }
    } else {
        const uint8_t count = low_knock_noise_counter;
        if (count == 0) {
            knock_low_noise_fault = 0;
        } else {
            low_knock_noise_counter = count - 1;
        }
    }
}
