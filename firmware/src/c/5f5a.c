#include <stdint.h>

/* Additional idle air control steps above the coolant threshold. */
volatile __code const uint8_t ac_hot_iac_extra_steps = 15;
