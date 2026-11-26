/* Bit address 0x08 maps to internal RAM 0x21, bit 0. */
static __sbit __at (0x08) flag_21_0;

void clear_flag_21_0_f275(void)
{
    flag_21_0 = 0;
}
