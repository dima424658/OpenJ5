#include <stdint.h>

/* IRAM lists contain byte addresses. XDATA lists contain high/low
 * address pairs. Both formats stop at a zero first byte. */
#define ADDRESS_BE(address) { (address) >> 8, (address) & 0xff }

__code const uint8_t diag_full_iram[] = {
    0x20, 0x21, 0x2b, 0x2c, 0x5b, 0x23, 0x24, 0x25,
    0x49, 0x61, 0x52, 0x55, 0x53, 0x64, 0x65, 0x6f, 0,
};

__code const uint8_t diag_full_xdata_bytes[][2] = {
    ADDRESS_BE(0xf844), ADDRESS_BE(0xf8ae), ADDRESS_BE(0xf978),
    ADDRESS_BE(0xf849), ADDRESS_BE(0xf815), ADDRESS_BE(0xf810), ADDRESS_BE(0),
};

__code const uint8_t diag_full_xdata_words[][2] = {
    ADDRESS_BE(0xf841), ADDRESS_BE(0xf808), ADDRESS_BE(0xf82e),
    ADDRESS_BE(0xf862), ADDRESS_BE(0xf874), ADDRESS_BE(0xf8f8), ADDRESS_BE(0),
};

__code const uint8_t diag_short_iram[] = {
    0x20, 0x21, 0x2b, 0x2c, 0x49, 0x61, 0x52, 0x55, 0x53, 0x65, 0x6f, 0,
};

__code const uint8_t diag_short_xdata_bytes[][2] = {
    ADDRESS_BE(0xf844), ADDRESS_BE(0xf8ae), ADDRESS_BE(0xf978),
    ADDRESS_BE(0xf810), ADDRESS_BE(0),
};

__code const uint8_t diag_short_xdata_words[][2] = {
    ADDRESS_BE(0xf841), ADDRESS_BE(0xf808), ADDRESS_BE(0xf82e),
    ADDRESS_BE(0xf862), ADDRESS_BE(0xf874), ADDRESS_BE(0),
};

__code const uint8_t diag_adc_iram[] = { 0x76, 0 };

__code const uint8_t diag_adc_xdata_bytes[][2] = {
    ADDRESS_BE(0xf80b), ADDRESS_BE(0xf80c), ADDRESS_BE(0xf80d), ADDRESS_BE(0xf815),
    ADDRESS_BE(0xf8a9), ADDRESS_BE(0xf8b3), ADDRESS_BE(0xf8b6), ADDRESS_BE(0xf8b7),
    ADDRESS_BE(0xf8b5), ADDRESS_BE(0),
};

__code const uint8_t diag_misfire_xdata_bytes[][2] = {
    ADDRESS_BE(0xfa33), ADDRESS_BE(0xfa34), ADDRESS_BE(0xfa4d), ADDRESS_BE(0xfa4e),
    ADDRESS_BE(0xfa4f), ADDRESS_BE(0xfa50), ADDRESS_BE(0xfa51), ADDRESS_BE(0xfa52),
    ADDRESS_BE(0xfa53), ADDRESS_BE(0xfa54), ADDRESS_BE(0xfa55), ADDRESS_BE(0xfa56),
    ADDRESS_BE(0xfa57), ADDRESS_BE(0xfa58), ADDRESS_BE(0xfa59), ADDRESS_BE(0xfa5a),
    ADDRESS_BE(0xf8c6), ADDRESS_BE(0xf8c7), ADDRESS_BE(0xf8c8), ADDRESS_BE(0xf8c9),
    ADDRESS_BE(0xf8ca), ADDRESS_BE(0xf8cb), ADDRESS_BE(0xf8cc), ADDRESS_BE(0xf8cd),
    ADDRESS_BE(0xf93d), ADDRESS_BE(0),
};

__code const char diag_software_identifier[16] = "J5V05L19        ";

#undef ADDRESS_BE
