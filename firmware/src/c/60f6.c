#include <stdint.h>

/* Maximum position in steps during cold closed-loop control. */
volatile __code const uint8_t iac_cold_max_position = 120;

/* Cold-start feedback threshold in the coolant code T + 40. */
volatile __code const uint8_t idle_iac_cold_temp_limit = -17 + 40;
