#include <stdint.h>

/* Raw gains for the proportional and integral terms. */
volatile __code const uint8_t iac_feedback_proportional_gain = 16;
volatile __code const uint8_t iac_feedback_integral_gain = 79;
