#include <stdint.h>

#define BE16(value) { (value) >> 8, (value) & 0xff }

/* Monotone 65-point curve. The image stores each word most significant byte first. */
__code const uint8_t unsigned_curve_756d_be[65][2] = {
    BE16(   0), BE16(   2), BE16(   5), BE16(   7), BE16(   9), BE16(  12),
    BE16(  19), BE16(  26), BE16(  34), BE16(  42), BE16(  52), BE16(  63),
    BE16(  76), BE16(  90), BE16( 107), BE16( 125), BE16( 145), BE16( 168),
    BE16( 192), BE16( 219), BE16( 248), BE16( 279), BE16( 314), BE16( 351),
    BE16( 393), BE16( 438), BE16( 487), BE16( 540), BE16( 595), BE16( 654),
    BE16( 717), BE16( 784), BE16( 855), BE16( 930), BE16(1010), BE16(1093),
    BE16(1179), BE16(1270), BE16(1367), BE16(1470), BE16(1581), BE16(1699),
    BE16(1825), BE16(1960), BE16(2102), BE16(2251), BE16(2406), BE16(2568),
    BE16(2735), BE16(2909), BE16(3088), BE16(3273), BE16(3465), BE16(3664),
    BE16(3869), BE16(4082), BE16(4302), BE16(4530), BE16(4766), BE16(5009),
    BE16(5262), BE16(5522), BE16(5792), BE16(6071), BE16(6359),
};

#undef BE16
