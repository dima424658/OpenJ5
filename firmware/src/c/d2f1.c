/* Bit address 0x7F maps to internal RAM 0x2F, bit 7. */
static __sbit __at (0x7f) flag_2f_7;

void set_flag_2f_7_d2f1(void)
{
    flag_2f_7 = 1;
}
