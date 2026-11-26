#include <stdint.h>

/* Cold-start position in steps and coolant temperature code T + 40. */
volatile __code const uint8_t cold_start_iac_max_position = 120;
volatile __code const uint8_t cold_start_coolant_threshold = -7 + 40;
