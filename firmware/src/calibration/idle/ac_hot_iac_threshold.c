#include <stdint.h>

/* Coolant threshold for the extra A/C stepper offset. */
volatile __code const uint8_t ac_hot_iac_threshold = 105;
