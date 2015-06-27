/* Copyright (C) 2000-2007 Analog Devices Inc., All Rights Reserved.
** This contains Development IP as defined in the ADI/Intel 
** Collaboration Agreement (ADI/Intel Confidential)
*/ 

/***************************************************************
   Func Name:    cadd_fr16

   Description:  addition of two complex numbers

***************************************************************/

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
#pragma file_attr("libGroup =complex.h")
#pragma file_attr("libFunc  =__cadd_fr16")
#pragma file_attr("libFunc  =cadd_fr16")   

#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")
#endif


#include <complex.h>
#include <builtins.h>

complex_fract16 
cadd_fr16(complex_fract16 a, complex_fract16 b)
{
    complex_fract16 result;

    result.re = __builtin_add_fr1x16( a.re, b.re );
    result.im = __builtin_add_fr1x16( a.im, b.im );

    return (result);
}

/* end of file */
