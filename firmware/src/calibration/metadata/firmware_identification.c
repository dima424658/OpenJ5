#include <stddef.h>
#include <stdint.h>

struct CalibrationHeader {
    uint8_t prefix_6000[10];
    char ecu_family[6];
    uint8_t field_6010[2];
    uint8_t padding_6012[14];
    char software_id[16];
    char timestamp[16];
    char engine_model[16];
    uint8_t byte_6050;
};

_Static_assert(offsetof(struct CalibrationHeader, ecu_family) == 0x0a, "ECU family offset");
_Static_assert(offsetof(struct CalibrationHeader, software_id) == 0x20, "Software ID offset");
_Static_assert(offsetof(struct CalibrationHeader, timestamp) == 0x30, "Timestamp offset");
_Static_assert(offsetof(struct CalibrationHeader, engine_model) == 0x40, "Engine model offset");
_Static_assert(sizeof(struct CalibrationHeader) == 0x51, "Calibration header size");

__code const struct CalibrationHeader calibration_header = {
    .ecu_family = "J5.1",
    .field_6010 = { 0x00, '0' },
    .software_id = "J5V05L19",
    .timestamp = "08/05/2003 12:00",
    .engine_model = "2112",
    .byte_6050 = 0x12,
};
