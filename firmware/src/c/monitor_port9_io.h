#ifndef JANVAR_MONITOR_PORT9_IO_H
#define JANVAR_MONITOR_PORT9_IO_H

#define ROTATE_PORT9_BIT()                        \
    do {                                          \
        const uint8_t carry_in = CY ? 0x80 : 0;   \
        CY = ACC & 1;                             \
        ACC = (ACC >> 1) | carry_in;              \
    } while (0)

/* Send on P9.3 and sample P9.6 while P9.0 is high. */
#define SHIFT_PORT9_BYTE()                        \
    do {                                          \
        B = 8;                                    \
        do {                                      \
            ROTATE_PORT9_BIT();                   \
            if (CY) {                             \
                P9 |= 0x08;                       \
            } else {                              \
                P9 &= 0xf7;                       \
            }                                     \
            P9 |= 1;                              \
            CY = (P9 & 0x40) != 0;                \
            P9 &= 0xfe;                           \
        } while (--B);                            \
        ROTATE_PORT9_BIT();                       \
    } while (0)

#endif
