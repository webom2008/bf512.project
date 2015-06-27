/**************************************************************************
 *
 * embc_rt.h : $Revision: 1.4.14.1 $
 * (c) Copyright 2008-2009 Analog Devices Inc. All rights reserved.
 *
 *************************************************************************/
#pragma once

#include <stdfix.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef _MISRA_RULES
#pragma diag(suppress:misra_rule_6_3)
/* Suppress Rule 6.3 (advisory) whereby typedefs that indicate
** size and signedness should be used in place of the basic types.
**
** Rationale: The C/C++ manual documents the prototypes of the library
** functions using the intrinsic C data types and these prototypes must
** correspond to the actual prototypes that are defined in the header
** files. In addition, Rule 8.3 requires that the type of the result
** and the type of the parameters that are specified in a function
** definition and a function declaration are identical.
*/
#endif

#ifdef __SYSCALL_INFO__
%syscall
%notes "Divides 1.15 fractional by 1.15 fractional with rounding."
#endif
#pragma const
#pragma regs_clobbered "R0-R3,P0-P2,LC0,LT0,LB0,CCset"
#pragma linkage_name ___divrr_rnd
fract divrr_rnd(fract _dividend, fract _divisor);

#ifdef __SYSCALL_INFO__
%syscall
%notes "Divides 0.16 fractional by 0.16 fractional with rounding."
#endif
#pragma const
#pragma regs_clobbered "R0-R3,P0-P2,LC0,LT0,LB0,CCset"
#pragma linkage_name ___divurur_rnd
unsigned fract divurur_rnd(unsigned fract _dividend,
                           unsigned fract _divisor);

#ifdef __SYSCALL_INFO__
%syscall
%notes "Converts single-precision floating-point number to 1.15 " \
       "fractional number with rounding."
#endif
#pragma const
#pragma regs_clobbered "R0-R3,P1,CCset"
#pragma linkage_name ___float_to_fr16_rnd
fract float_to_fr16_rnd(float parm1);

#ifdef __SYSCALL_INFO__
%syscall
%notes "Converts single-precision floating-point number to 40-bit accum."
#endif
#pragma const
#pragma regs_clobbered "R0-R3,P0-P1,CCset"
#pragma linkage_name ___float_to_accum
accum float_to_accum(float parm1);

#ifdef __SYSCALL_INFO__
%syscall
%notes "Converts single-precision floating-point number to 40-bit accum " \
       "with rounding."
#endif
#pragma const
#pragma regs_clobbered "R0-R3,P0-P1,CCset"
#pragma linkage_name ___float_to_accum_rnd
accum float_to_accum_rnd(float parm1);

#ifdef __SYSCALL_INFO__
%syscall
%notes "Converts single-precision floating-point number to 0.32 " \
       "fractional number."
#endif
#pragma const
#pragma regs_clobbered "R0-R3,P0-P1,CCset"
#pragma linkage_name ___float_to_unsigned_fr32
unsigned long fract float_to_unsigned_fr32(float parm1);

#ifdef __SYSCALL_INFO__
%syscall
%notes "Converts single-precision floating-point number to 0.16 " \
       "fractional number."
#endif
#pragma const
#pragma regs_clobbered "R0-R3,P0-P1,CCset"
#pragma linkage_name ___float_to_unsigned_fr16
unsigned fract float_to_unsigned_fr16(float parm1);

#ifdef __SYSCALL_INFO__
%syscall
%notes "Converts single-precision floating-point number to 0.16 " \
       "fractional number with rounding."
#endif
#pragma const
#pragma regs_clobbered "R0-R3,P0-P1,CCset"
#pragma linkage_name ___float_to_unsigned_fr16_rnd
unsigned fract float_to_unsigned_fr16_rnd(float parm1);

#ifdef __SYSCALL_INFO__
%syscall
%notes "Converts single-precision floating-point number to 40-bit " \
       "unsigned accum."
#endif
#pragma const
#pragma regs_clobbered "R0-R3,P0-P1,CCset"
#pragma linkage_name ___float_to_unsigned_accum
unsigned accum float_to_unsigned_accum(float parm1);

#ifdef __SYSCALL_INFO__
%syscall
%notes "Converts single-precision floating-point number to 40-bit " \
       "unsigned accum with rounding."
#endif
#pragma const
#pragma regs_clobbered "R0-R3,P0-P1,CCset"
#pragma linkage_name ___float_to_unsigned_accum_rnd
unsigned accum float_to_unsigned_accum_rnd(float parm1);

#ifdef __SYSCALL_INFO__
%syscall
%notes "Converts double-precision floating-point number to 1.31 " \
       "fractional number."
#endif
#pragma const
#pragma regs_clobbered "R0-R3,P0-P1,CCset"
#pragma linkage_name ___float64_to_fr32
long fract float64_to_fr32(long double parm1);

#ifdef __SYSCALL_INFO__
%syscall
%notes "Converts double-precision floating-point number to 1.15 " \
       "fractional number."
#endif
#pragma const
#pragma regs_clobbered "R0-R3,P0-P1,CCset"
#pragma linkage_name ___float64_to_fr16
fract float64_to_fr16(long double parm1);

#ifdef __SYSCALL_INFO__
%syscall
%notes "Converts double-precision floating-point number to 1.15 " \
       "fractional number with rounding."
#endif
#pragma const
#pragma regs_clobbered "R0-R3,P0-P1,CCset"
#pragma linkage_name ___float64_to_fr16_rnd
fract float64_to_fr16_rnd(long double parm1);

#ifdef __SYSCALL_INFO__
%syscall
%notes "Converts double-precision floating-point number to 40-bit accum."
#endif
#pragma const
#pragma regs_clobbered "R0-R3,P0-P1,CCset"
#pragma linkage_name ___float64_to_accum
accum float64_to_accum(long double parm1);

#ifdef __SYSCALL_INFO__
%syscall
%notes "Converts double-precision floating-point number to 40-bit accum " \
       "with rounding."
#endif
#pragma const
#pragma regs_clobbered "R0-R3,P0-P1,CCset"
#pragma linkage_name ___float64_to_accum_rnd
accum float64_to_accum_rnd(long double parm1);

#ifdef __SYSCALL_INFO__
%syscall
%notes "Converts double-precision floating-point number to 0.32 " \
       "fractional number."
#endif
#pragma const
#pragma regs_clobbered "R0-R3,P0-P1,CCset"
#pragma linkage_name ___float64_to_unsigned_fr32
unsigned long fract float64_to_unsigned_fr32(long double parm1);

#ifdef __SYSCALL_INFO__
%syscall
%notes "Converts double-precision floating-point number to 0.16 " \
       "fractional number."
#endif
#pragma const
#pragma regs_clobbered "R0-R3,P0-P1,CCset"
#pragma linkage_name ___float64_to_unsigned_fr16
unsigned fract float64_to_unsigned_fr16(long double parm1);

#ifdef __SYSCALL_INFO__
%syscall
%notes "Converts double-precision floating-point number to 0.16 " \
       "fractional number with rounding."
#endif
#pragma const
#pragma regs_clobbered "R0-R3,P0-P1,CCset"
#pragma linkage_name ___float64_to_unsigned_fr16_rnd
unsigned fract float64_to_unsigned_fr16_rnd(long double parm1);

#ifdef __SYSCALL_INFO__
%syscall
%notes "Converts double-precision floating-point number to 40-bit " \
       "unsigned accum."
#endif
#pragma const
#pragma regs_clobbered "R0-R3,P0-P1,CCset"
#pragma linkage_name ___float64_to_unsigned_accum
unsigned accum float64_to_unsigned_accum(long double parm1);

#ifdef __SYSCALL_INFO__
%syscall
%notes "Converts double-precision floating-point number to 40-bit " \
       "unsigned accum with rounding."
#endif
#pragma const
#pragma regs_clobbered "R0-R3,P0-P1,CCset"
#pragma linkage_name ___float64_to_unsigned_accum_rnd
unsigned accum float64_to_unsigned_accum_rnd(long double parm1);

#pragma regs_clobbered "A0,A1,R0-R3,P0-P1,CCset"
#pragma linkage_name ___mul48x48_96
void mul48x48_96(int64_t a, int64_t b, uint32_t *bits_0_31,
                 uint32_t *bits_32_63, int32_t *bits_64_95);

#ifdef __SYSCALL_INFO__
%syscall
%notes "Multiplies a 48-bit integer by an accum to give an accum."
#endif
#pragma const
#pragma regs_clobbered "A0,A1,R0-R3,P0-P1,CCset"
#pragma linkage_name ___muli48k
accum muli48k(int64_t a, accum b);

#ifdef __SYSCALL_INFO__
%syscall
%notes "40-bit signed accum multiplication."
#endif
#pragma const
#pragma regs_clobbered "A0,A1,R0-R3,P0-P1,CCset"
#pragma linkage_name ___mulkk
accum mulkk(accum a, accum b, bool rnd_to_nearest);

#ifdef __SYSCALL_INFO__
%syscall
%notes "Multiplies a 48-bit unsigned integer by an unsigned accum to give " \
       "an unsigned accum."
#endif
#pragma const
#pragma regs_clobbered "A0,A1,R0-R3,P0-P1,CCset"
#pragma linkage_name ___mului48uk
unsigned accum mului48uk(uint64_t a, unsigned accum b);

#ifdef __SYSCALL_INFO__
%syscall
%notes "40-bit unsigned accum multiplication."
#endif
#pragma const
#pragma regs_clobbered "A0,A1,R0-R3,P0-P1,CCset"
#pragma linkage_name ___mulukuk
unsigned accum mulukuk(unsigned accum a, unsigned accum b,
                       bool rnd_to_nearest);

/* End of file */
