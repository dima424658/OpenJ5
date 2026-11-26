#include <stdint.h>

/* Initial idle air control steps when requesting A/C. */
volatile __code const uint8_t ac_iac_offset_steps = 30;
