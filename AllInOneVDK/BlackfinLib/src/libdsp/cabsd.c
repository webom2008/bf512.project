/************************************************************************
 *
 * cabsd.c : $Revision: 4 $
 *
 * (c) Copyright 2004 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
 * Description :   Absolute value of a complex number.
 *                   cabs(a) = sqrt( a.re^2 + a.im^2 )
 */


#pragma file_attr("libGroup =complex.h")
#pragma file_attr("libFunc  =cabsd")
#pragma file_attr("libFunc  =__cabsd")     //from complex.h
#pragma file_attr("libFunc  =cabs")        //from complex.h

/* Called by normd */
#pragma file_attr("libFunc  =normd")
#pragma file_attr("libFunc  =__normd")     //from complex.h
#pragma file_attr("libFunc  =norm")        //from complex.h

/* Called by cartesiand */
#pragma file_attr("libFunc  =cartesiand")
#pragma file_attr("libFunc  =__cartesiand")     //from complex.h
#pragma file_attr("libFunc  =cartesian")        //from complex.h

#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

/* Defined in */
#include <complex.h>

#include <math.h>


long double                       /*{ ret - Absolute value         }*/
cabsd
(
  complex_long_double  a          /*{ (i) - Complex input `a`      }*/
)
{

    long double   x;
    long double   abs_re, abs_im, tmp, tmp2;

    /* 
       To prevent avoidable overflows, underflow or loss of precision,
       the following alternative algorithm is used:

       If |a.re| >= |a.im|
         |a.re| * sqrt( 1 + (a.im/a.re)^2 ) 
       Else    // |a.re| < |a.im|
         |a.im| * sqrt( 1 + (a.re/a.im)^2 )
     */

    if( (a.re == 0) && (a.im == 0) )
    {
       /* c = sqrt( 0 + 0 ) = 0 */
       x = 0.0L;
    } 
    else
    {
       abs_re = fabsd( a.re );
       abs_im = fabsd( a.im );
       
       if (a.re == 0)
       {
          /* c = sqrt( 0 + a.im^2 ) = |a.im| */
          x = abs_im;
       }
       else if (a.im == 0)
       {
          /* c = sqrt( a.re^2 + 0 ) = |a.re| */
          x = abs_re;
       }
       else 
       { 
          if( abs_re >= abs_im )
          {
             tmp  = a.im/a.re;
             tmp2 = abs_re;
          }
          else
          {
             tmp  = a.re/a.im;
             tmp2 = abs_im;
          }
          x = tmp2 * sqrtd( 1.0L + (tmp * tmp) );
       }
    }

    return (x);
}
