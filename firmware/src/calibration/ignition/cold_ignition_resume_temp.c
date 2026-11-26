#include <stdint.h>

/* Coolant temperature in degrees Celsius plus 40. */
volatile __code const uint8_t cold_ignition_resume_temp = 60 + 40;
volatile __code const uint8_t cold_ignition_disable_temp = 68 + 40;
