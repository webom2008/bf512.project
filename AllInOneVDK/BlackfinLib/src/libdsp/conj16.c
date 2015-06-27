/* Copyright (C) 2000-2007 Analog Devices Inc., All Rights Reserved.
** This contains Development IP as defined in the ADI/Intel 
** Collaboration Agreement (ADI/Intel Confidential)
*/

/**************************************************************
   Func Name:    conj_fr16

   Description:  conjungate the complex input
                    re(result) = re(a)
                    im(result) = - im(a)

**************************************************************/

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
#pragma file_attr("libGroup =complex.h")
#pragma file_attr("libFunc  =conj_fr16")
#pragma file_attr("libFunc  =__conj_fr16")

#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")
#endif

#include <complex.h>
#include <builtins.h>

complex_fract16 
conj_fr16 (complex_fract16 a)
{	
    complex_fract16 result = a;
    int             negate_a;

    negate_a = __builtin_negate_fr2x16(
                         __builtin_compose_2x16(a.im, a.re)
                                      );
    result.im = __builtin_extract_hi(negate_a);

    return (result); 
}

/* end of file */
