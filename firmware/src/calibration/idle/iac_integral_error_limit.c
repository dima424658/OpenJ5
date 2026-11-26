#include <stdint.h>

/* Limit the integral input to 200 RPM in units of 10 RPM. */
volatile __code const uint8_t iac_integral_error_limit = 20;
