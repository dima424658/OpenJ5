#include <stdint.h>

#define ECU_NUMBER "2112 -1411020-41"
#define SUPPLIER_HARDWARE "NOTSUPPORT"
#define SUPPLIER_SOFTWARE "1411010-41"
#define ENGINE_TYPE "SAMARA-1.5L,16V"
#define PROGRAMMING_DATE "08-05-2003"
#define SOFTWARE_ID "J5V05L19"

__code const char stock_ecu_number[] = ECU_NUMBER;
__code const char stock_supplier_hardware[] = SUPPLIER_HARDWARE;
__code const char stock_supplier_software[] = SUPPLIER_SOFTWARE;
__code const char stock_engine_type[] = ENGINE_TYPE;
__code const char stock_programming_date[] = PROGRAMMING_DATE;
__code const char stock_software_id[] = SOFTWARE_ID;
__code const char stock_supplier_software_copy[] = SUPPLIER_SOFTWARE;

/* Format counts the service byte and its data. Every template ends with
 * the eight-bit sum and a CODE string terminator. */
#define FIXED_REPLY_1(name, service)                                        \
    __code const uint8_t name[] = {                                         \
        0x81, 0xf1, 0x10, (service),                                        \
        (uint8_t)(0x81u + 0xf1u + 0x10u + (service)), 0,                    \
    }

#define FIXED_REPLY_2(name, service, value)                                 \
    __code const uint8_t name[] = {                                         \
        0x82, 0xf1, 0x10, (service), (value),                               \
        (uint8_t)(0x82u + 0xf1u + 0x10u + (service) + (value)), 0,          \
    }

#define FIXED_REPLY_3(name, service, first, second)                         \
    __code const uint8_t name[] = {                                         \
        0x83, 0xf1, 0x10, (service), (first), (second),                     \
        (uint8_t)(0x83u + 0xf1u + 0x10u + (service) + (first) + (second)), 0,\
    }

FIXED_REPLY_3(link_start_reply, 0xc1, 0x6b, 0x8f);
FIXED_REPLY_1(link_stop_reply, 0xc2);
FIXED_REPLY_2(session_start_reply, 0x50, 0x81);
FIXED_REPLY_1(session_stop_reply, 0x60);
/* Zero is the checksum here. A string append stops before it. */
FIXED_REPLY_1(tester_present_reply, 0x7e);
FIXED_REPLY_1(service_11_reply, 0x51);
/* These two templates are copied with an explicit byte count. */
FIXED_REPLY_3(faults_cleared_group_00, 0x54, 0, 0);
FIXED_REPLY_3(faults_cleared_group_ff, 0x54, 0xff, 0);

#define NEGATIVE_REPLY(name, service, reason) FIXED_REPLY_3(name, 0x7f, service, reason)

NEGATIVE_REPLY(link_stop_inactive_reply, 0x82, 0x10);
NEGATIVE_REPLY(session_start_denied_reply, 0x10, 0x10);
NEGATIVE_REPLY(session_stop_inactive_reply, 0x20, 0x10);
NEGATIVE_REPLY(service_11_inactive_reply, 0x11, 0x10);
NEGATIVE_REPLY(fault_clear_inactive, 0x14, 0x10);
NEGATIVE_REPLY(fault_read_inactive, 0x18, 0x10);
NEGATIVE_REPLY(identification_inactive_reply, 0x1a, 0x10);
NEGATIVE_REPLY(diag_inactive_response, 0x21, 0x10);
NEGATIVE_REPLY(service_23_inactive_reply, 0x23, 0x10);
NEGATIVE_REPLY(output_unavailable_reply, 0x30, 0x10);
NEGATIVE_REPLY(service_3b_inactive_reply, 0x3b, 0x10);
NEGATIVE_REPLY(service_3e_inactive_reply, 0x3e, 0x10);
NEGATIVE_REPLY(session_mode_invalid_reply, 0x10, 0x12);
NEGATIVE_REPLY(fault_clear_invalid_group, 0x14, 0x12);
NEGATIVE_REPLY(identification_invalid_reply, 0x1a, 0x12);
NEGATIVE_REPLY(fault_read_invalid_group, 0x18, 0x12);
NEGATIVE_REPLY(diag_unknown_parameter, 0x21, 0x12);
NEGATIVE_REPLY(service_23_invalid_reply, 0x23, 0x12);
NEGATIVE_REPLY(service_3b_invalid_reply, 0x3b, 0x12);
NEGATIVE_REPLY(output_invalid_reply, 0x30, 0x12);
NEGATIVE_REPLY(service_23_range_reply, 0x23, 0x31);
NEGATIVE_REPLY(output_range_reply, 0x30, 0x31);

/* The combined identification answer appends fields and its sum at runtime. */
__code const uint8_t identification_prefix[] = { 0x80, 0xf1, 0x10, 0x61, 0x5a, 0x80, 0 };

/* The final literal before the terminator is the stock frame checksum. */
__code const uint8_t identification_91_reply[] = "\x92\xf1\x10\x5a\x91" ECU_NUMBER "\x7c";
__code const uint8_t identification_92_reply[] = "\x8c\xf1\x10\x5a\x92" SUPPLIER_HARDWARE "\xa7";
__code const uint8_t identification_94_reply[] = "\x8c\xf1\x10\x5a\x94" SUPPLIER_SOFTWARE "\x65";
__code const uint8_t identification_97_reply[] = "\x91\xf1\x10\x5a\x97" ENGINE_TYPE "\x2e";
__code const uint8_t identification_99_reply[] = "\x8c\xf1\x10\x5a\x99" PROGRAMMING_DATE "\x6c";
__code const uint8_t identification_9a_reply[] = "\x8a\xf1\x10\x5a\x9a" SOFTWARE_ID "\x6f";
