/* C509-L Data Pointer Select Register. */
static __sfr __at (0x92) DPSEL;

void select_data_pointer_zero(void)
{
    DPSEL = 0;
}
