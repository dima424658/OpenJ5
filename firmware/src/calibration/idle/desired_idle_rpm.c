#include <stdint.h>

#define IDLE_RPM_TO_CODE(rpm) ((rpm) / 10)

/* Coolant temperature from -40 to 150 deg C in steps of 5 deg C. */
__code const uint8_t desired_idle_rpm[39] = {
    IDLE_RPM_TO_CODE(1600), IDLE_RPM_TO_CODE(1590), IDLE_RPM_TO_CODE(1570),
    IDLE_RPM_TO_CODE(1550), IDLE_RPM_TO_CODE(1530), IDLE_RPM_TO_CODE(1500),
    IDLE_RPM_TO_CODE(1470), IDLE_RPM_TO_CODE(1440), IDLE_RPM_TO_CODE(1410),
    IDLE_RPM_TO_CODE(1380), IDLE_RPM_TO_CODE(1350), IDLE_RPM_TO_CODE(1330),
    IDLE_RPM_TO_CODE(1300), IDLE_RPM_TO_CODE(1290), IDLE_RPM_TO_CODE(1280),
    IDLE_RPM_TO_CODE(1270), IDLE_RPM_TO_CODE(1220), IDLE_RPM_TO_CODE(1170),
    IDLE_RPM_TO_CODE(1110), IDLE_RPM_TO_CODE(1050), IDLE_RPM_TO_CODE(990),
    IDLE_RPM_TO_CODE(940), IDLE_RPM_TO_CODE(890), IDLE_RPM_TO_CODE(850),
    IDLE_RPM_TO_CODE(820), IDLE_RPM_TO_CODE(800), IDLE_RPM_TO_CODE(800),
    IDLE_RPM_TO_CODE(800), IDLE_RPM_TO_CODE(800), IDLE_RPM_TO_CODE(800),
    IDLE_RPM_TO_CODE(830), IDLE_RPM_TO_CODE(870), IDLE_RPM_TO_CODE(910),
    IDLE_RPM_TO_CODE(950), IDLE_RPM_TO_CODE(990), IDLE_RPM_TO_CODE(990),
    IDLE_RPM_TO_CODE(990), IDLE_RPM_TO_CODE(990), IDLE_RPM_TO_CODE(990),
};

/* Idle timing in half-degrees versus half-bin RPM index. */
__code const int8_t idle_ignition_by_rpm[32] = {
      36,   36,   36,   36,   36,   36,   36,   36,   36,   36,   36,   36,   36,   37,   38,   39,
      40,   41,   42,   43,   44,   45,   46,   47,   48,   49,   50,   51,   52,   53,   54,   54,
};

/* Correction subtracted from idle timing, in half-degrees versus coolant temperature. */
__code const int8_t idle_ignition_temp_correction[39] = {
     -30,  -30,  -29,  -27,  -25,  -23,  -21,  -19,  -17,  -14,  -11,   -8,   -5,   -2,    0,    0,
       0,    0,    0,    0,    2,    4,    6,    8,   10,   10,   10,   10,   10,   10,   10,   10,
      10,   10,   10,   10,   10,   10,   10,
};
