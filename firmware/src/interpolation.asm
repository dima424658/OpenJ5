        .globl interp_bilinear_u8
        .globl interp_bilinear_end

        .org 0xa59d
; DPTR = CODE table, R2 = 4.4 column coordinate, R3 = 4.4 row coordinate.
; A = interpolated unsigned byte. B, R0-R4 and arithmetic flags are clobbered.
; The returned byte is the high byte of the weighted sum. Index arithmetic uses the stock
; 8-bit wraparound; the caller supplies the axes.
interp_bilinear_u8:
        mov     a,r3
        anl     a,#0xf0
        mov     b,a
        mov     a,r2
        anl     a,#0xf0
        swap    a
        add     a,b
        mov     r4,a
        mov     a,r2
        anl     a,#0x0f
        mov     r2,a
        mov     a,r3
        anl     a,#0x0f
        mov     r3,a
        orl     a,r2
        jnz     bilinear_weighted
        mov     a,r4
        movc    a,@a+dptr
        ret

bilinear_weighted:
        ; Bottom-right node times fx * fy.
        mov     a,r2
        mov     b,r3
        mul     ab
        mov     b,a
        mov     a,r4
        add     a,#0x11
        movc    a,@a+dptr
        mul     ab
        mov     r0,a
        mov     r1,b

        ; Bottom-left node times (16 - fx) * fy.
        mov     a,#0x10
        clr     c
        subb    a,r2
        mov     b,r3
        mul     ab
        mov     b,a
        mov     a,r4
        add     a,#0x10
        movc    a,@a+dptr
        mul     ab
        add     a,r0
        mov     r0,a
        mov     a,r1
        addc    a,b
        mov     r1,a

        ; Top-right node times fx * (16 - fy).
        mov     a,#0x10
        clr     c
        subb    a,r3
        mov     r3,a
        mov     b,r2
        mul     ab
        mov     b,a
        mov     a,r4
        inc     a
        movc    a,@a+dptr
        mul     ab
        add     a,r0
        mov     r0,a
        mov     a,r1
        addc    a,b
        mov     r1,a

        ; Top-left node times (16 - fx) * (16 - fy).
        mov     a,#0x10
        clr     c
        subb    a,r2
        mov     b,r3
        mul     ab
        mov     b,a
        mov     a,r4
        movc    a,@a+dptr
        mul     ab
        add     a,r0
        mov     r0,a
        mov     a,r1
        addc    a,b
        ret
interp_bilinear_end:

; CODE [0xa611, 0xa62f) comes from src/c/a611.c.
