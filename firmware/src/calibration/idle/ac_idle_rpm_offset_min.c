#include <stdint.h>

/* Idle-speed offset with the air conditioning active, in tens of rpm. */
volatile __code const uint8_t ac_idle_rpm_offset_min = 10;
