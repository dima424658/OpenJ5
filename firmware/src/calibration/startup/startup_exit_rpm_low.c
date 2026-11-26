#include <stdint.h>

/* RPM threshold for beginning the transition out of startup, in tens of RPM. */
volatile __code const uint8_t startup_exit_rpm_low = 40;
