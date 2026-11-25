#include <stdint.h>

/* Cold-start feedback threshold in the coolant code T + 40. */
volatile __code const uint8_t idle_iac_cold_temp_limit = -17 + 40;
