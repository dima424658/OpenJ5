// The linker places one tail call in each active vector slot.
extern void STOCK_VECTOR_TARGET(void);

void STOCK_VECTOR_ENTRY(void)
{
    STOCK_VECTOR_TARGET();
}
