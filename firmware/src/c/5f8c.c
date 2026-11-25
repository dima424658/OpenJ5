#include <stdint.h>

/* Stepper position used when the throttle position sensor reports a fault. */
volatile __code const uint8_t iac_throttle_fault_position = 70;
