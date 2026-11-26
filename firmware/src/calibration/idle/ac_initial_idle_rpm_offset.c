#include <stdint.h>

/* Initial idle-speed offset for A/C, in tens of RPM. */
volatile __code const uint8_t ac_initial_idle_rpm_offset = 120;
