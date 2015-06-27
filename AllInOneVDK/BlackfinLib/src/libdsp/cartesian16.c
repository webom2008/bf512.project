/***************************************************************************
 *
 * cartesian16.c : $Revision: 4 $
 *
 * Copyright (c) 2003 Analog Devices Inc. All rights reserved.
 *
 ***************************************************************************/

/* This function converts a complex number 
   from cartesian to polar notation. 
 */

#pragma file_attr("libGroup =complex.h")
#pragma file_attr("libFunc  =__cartesian_fr16")
#pragma file_attr("libFunc  =cartesian_fr16")

#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

#include <complex.h>

fract16 _cartesian_fr16 ( complex_fract16 a, fract16* phase )
{
   *phase = arg_fr16(a);      /* compute phase     */
   return( cabs_fr16(a) );    /* compute magnitude */
}

