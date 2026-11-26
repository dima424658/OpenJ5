#include <stdint.h>

/* Full exit from startup mode, in tens of RPM, indexed by coolant temperature. */
__code const uint8_t startup_exit_rpm_by_coolant[39] = {
     82,  82,  82,  82,  82,  82, 100, 100, 100, 100, 100, 100, 100,
    100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
    100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
};
