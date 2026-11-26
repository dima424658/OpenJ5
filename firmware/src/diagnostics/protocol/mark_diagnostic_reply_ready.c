/* Bit address 0x7C maps to internal RAM 0x2F, bit 4. */
static __sbit __at (0x7c) flag_2f_4;

void set_flag_2f_4_cd7c(void)
{
    flag_2f_4 = 1;
}
