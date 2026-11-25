#include <stdint.h>

/* CTP GTCMIN, small cyclic fuel. J5 id 7592956F; raw step 2.84439215 mg/cycle. */
__code const uint8_t cranking_small_fuel_raw[39] = {
    113, 113, 109,  99,  88,  77,  67,  59,  53,  46,  40,  34,  29,  26,  24,  22,
     20,  18,  17,  15,  14,  13,  12,  10,   9,   8,   7,   7,   7,   7,   8,  11,
     11,  11,  11,  11,  11,  11,  11,
};

/* Cranking RPM correction. J5 id 547F7F3E. */
__code const uint8_t cranking_rpm_correction_raw[32] = {
    255, 255, 255, 255, 255, 255, 255, 255, 251, 246, 240, 234, 227, 219, 208, 193,
    175, 155, 131, 103,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,
};

/* CTP GTCA, asynchronous cyclic fuel. J5 id 21D35BE1; raw step 1.4222 mg/cycle. */
__code const uint8_t cranking_async_fuel_raw[39] = {
      0,   0,   0,   0,   0,  52, 101,  98,  95,  90,  84,  73,  61,  52,  46,  40,
     36,  31,  27,  23,  20,  18,  16,  14,  13,  12,  12,  12,  12,  12,  17,  17,
     17,  17,  17,  17,  17,  17,  17,
};

/* CTP GTCST, large cyclic fuel. J5 id 852388F5. */
__code const uint8_t cranking_large_fuel_raw[39] = {
    113, 113, 109,  99,  90,  82,  74,  67,  60,  52,  46,  39,  34,  29,  26,  24,
     22,  20,  18,  16,  14,  12,  10,   9,   8,   8,   8,   8,   8,   8,   9,  11,
     11,  11,  11,  11,  11,  11,  11,
};

/* Cranking ignition angle. J5 id 19A78759. */
__code const uint8_t cranking_ignition_raw[40] = {
      0,   0,   0,   0,   0,   1,   1,   2,   2,   3,   4,   5,   6,   7,   8,  10,
     12,  14,  16,  18,  21,  24,  27,  30,  36,  50,  50,  50,  50,  50,  50,  50,
     36,  36,  36,  36,  36,  36,  36,  36,
};

/* Ignition module dwell versus voltage. J5 id 47E275C8. */
__code const uint8_t ignition_dwell_raw[16] = {
    240, 206, 163, 133, 109,  93,  78,  67,  59,  51,  46,  42,  37,  34,  30,  29,
};

/* CTP KFd, injector dynamic offset versus voltage. J5 id E226DB35. */
__code const uint8_t injector_dynamic_offset_raw[16] = {
    250, 250, 242, 218, 185, 155, 131, 113,  94,  76,  63,  50,  41,  34,  29,  25,
};

/* Maximum ignition retard during knock correction. J5 id 5FAE0FA7. */
__code const uint8_t knock_maximum_retard_raw[32] = {
      0,   0,   0,   0,   6,   9,  12,  14,  15,  16,  17,  17,  18,  19,  20,  20,
     20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  19,  16,  12,   0,   0,   0,
};

/* Throttle position quantization (0 to 100%). J5 id 21B373DD. */
__code const uint8_t throttle_quantization_q4[101] = {
      0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  84,  88,  92,  96, 100,
    104, 108, 112, 115, 118, 122, 125, 128, 131, 133, 136, 139, 141, 144, 146, 148,
    150, 152, 154, 156, 158, 160, 162, 164, 165, 167, 169, 171, 172, 173, 176, 177,
    179, 181, 182, 184, 186, 187, 189, 190, 192, 194, 195, 197, 198, 200, 202, 204,
    205, 206, 208, 209, 210, 211, 213, 214, 215, 216, 217, 218, 219, 221, 222, 223,
    224, 225, 226, 227, 228, 229, 230, 231, 232, 232, 233, 234, 235, 236, 237, 238,
    239, 239, 240, 240, 240,
};
