/* Copyright (C) 2000-2007 Analog Devices Inc., All Rights Reserved.
** This contains Development IP as defined in the ADI/Intel 
** Collaboration Agreement (ADI/Intel Confidential)
*/

/******************************************************************
   Func Name:    cmlt_fr16

   Description:  multiplication of two complex numbers

******************************************************************/

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
#pragma file_attr("libGroup =complex.h")
#pragma file_attr("libFunc  =__cmlt_fr16")
#pragma file_attr("libFunc  =cmlt_fr16")

#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")
#endif

#include <complex.h>
#include <builtins.h>

complex_fract16 
cmlt_fr16 (complex_fract16 a, complex_fract16 b)
{
    complex_fract16 result;
    int             product;

    product = __builtin_cmplx_mul(
                    __builtin_compose_2x16(a.im, a.re),
                    __builtin_compose_2x16(b.im, b.re)
                                 ); 
    
    result.re = __builtin_extract_lo(product);
    result.im = __builtin_extract_hi(product);

    return (result);
}

/* end of file */
