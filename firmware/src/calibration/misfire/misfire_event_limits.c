#include <stdint.h>

/* Little-endian raw-event minimum, followed by the sample-count threshold. */
__code const uint8_t misfire_event_limits[4] = { 0x88, 0x13, 0xb8, 0x0b };
