#ifndef JANVAR_MONITOR_PORT9_CONFIG_H
#define JANVAR_MONITOR_PORT9_CONFIG_H

#define CONFIG_DELAY_8                               \
    __asm nop __endasm; __asm nop __endasm;          \
    __asm nop __endasm; __asm nop __endasm;          \
    __asm nop __endasm; __asm nop __endasm;          \
    __asm nop __endasm; __asm nop __endasm;

#define CONFIG_DELAY_32()                            \
    do {                                             \
        CONFIG_DELAY_8 CONFIG_DELAY_8                \
        CONFIG_DELAY_8 CONFIG_DELAY_8                \
    } while (0)

#define ROTATE_CONFIG_BIT()                          \
    do {                                             \
        const uint8_t carry_in = CY;                 \
        CY = ACC_SIGN;                               \
        ACC = (ACC << 1) | carry_in;                 \
    } while (0)

/* Data on P9.7, clock on P9.4 and frame select on P9.5. */
#define SEND_PORT9_CONFIG()                          \
    do {                                             \
        P9 &= 0xdf;                                  \
        B = 8;                                       \
        do {                                         \
            ROTATE_CONFIG_BIT();                     \
            if (CY) {                                \
                P9 |= 0x80;                          \
            } else {                                 \
                P9 &= 0x7f;                          \
            }                                        \
            P9 &= 0xef;                              \
            P9 |= 0x10;                              \
        } while (--B);                               \
        ROTATE_CONFIG_BIT();                         \
        P9 |= 0x20;                                  \
    } while (0)

#endif
