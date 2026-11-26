#include <stdint.h>

/* Coolant temperature is encoded as T + 40. */
volatile __code const uint8_t dynamic_ignition_temp_limit = 70 + 40;
