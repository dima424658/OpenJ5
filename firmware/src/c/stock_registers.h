#ifndef JANVAR_STOCK_REGISTERS_H
#define JANVAR_STOCK_REGISTERS_H

#include <stdint.h>

/* Operand aliases for stock_registers.peep. Rn in the linked image accesses
 * the current PSW register bank. These C declarations describe operand widths
 * and provide symbols for lowering, with no allocated storage or prologue.
 */
static volatile __data __at (0) uint8_t stock_r0;
static volatile __data __at (1) uint8_t stock_r1;
static volatile __data __at (2) uint8_t stock_r2;
static volatile __data __at (3) uint8_t stock_r3;

static volatile __data __at (0) uint16_t stock_r01;
static volatile __data __at (2) uint16_t stock_r23;

static __sfr __at (0xe0) ACC;
static __sbit __at (0xe7) ACC_SIGN;

#endif
