#include <stdint.h>

/* One-second reduction of the idle-speed offset, in tens of rpm. */
volatile __code const uint8_t ac_idle_rpm_offset_decay = 12;
