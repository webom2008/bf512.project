/*****************************************************************************
 *
 * cabs_fr32.c : $Revision: 1.5 $
 *
 * (c) Copyright 2009-2010 Analog Devices, Inc.  All rights reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)

    Function: cabs_fr32 - complex absolute value 


    Synopsis:

        #include <complex.h>
        fract32 cabs_fr32( complex_fract32 a );


    Description:

        The cabs_fr32 function computes the complex absolute value of 
        a complex input and returns the result.


    Error conditions:

        The cabs function does not return an error condition.


    Algorithm:

           abs_a = sqrt( a.real * a.real + a.imag * a.imag )

        There are two issues with the generic formula one needs to  
        be aware of:

           1) multiplying a small fractional number with itself 
              will cause underflow
           2) the sum of squares might exceed the valid range 
              of the fractional sqrt function

        First of, sort the real and imaginary value by size:
           min_val = min( |a.real|, |a.imag| )
           max_val = max( |a.real|, |a.imag| )

        Thus the above formula changes to:
           abs_a = sqrt( min_val^2 + max_val^2 )

        To address issue 1), compute
           abs_a = sqrt( (max_val^2 / max_val^2) * (min_val^2 + max_val^2) )
                 = sqrt( ((max_val^2 * min_val^2) / max_val^2) + 
                         ((max_val^2 * max_val^2) / max_val^2) )
                 = sqrt( max_val^2 * ((min_val^2 / max_val^2) + 
                                      (max_val^2 / max_val^2)) )
                 = sqrt( max_val^2 * ((min_val / max_val)^2 + 1) )
                 = max_val * sqrt( (min_val / max_val)^2 + 1 )

        The division operation will create a larger value to be squared, 
        thus reducing the risk of underflow.        

        A second transform is applied to address issue 2):
           abs_a = max_val * sqrt( (4/4) * ((min_val / max_val)^2 + 1) )
                 = max_val * sqrt( 4 * (((min_val / max_val)^2 + 1) / 4) )
                 = max_val * 2 * sqrt( (((min_val / max_val)^2) / 4) + 1/4 )

     Example:

        #include <complex.h>

        complex_fract32 a;
        fract32 dist;

        a.re = 0x20000000;          /* 0.5 * cos(pi/3)   */
        a.im = 0x376CF5D1;          /* 0.5 * sin(pi/3)   */
        dist = cabs_fr32 (a);       /* dist = 0x40000000 */


*****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
#pragma file_attr("libGroup=complex.h")
#pragma file_attr("libFunc=__cabs_fr32")
#pragma file_attr("libFunc=cabs_fr32")

/* Called by cartesian_fr32 */
#pragma file_attr("libFunc=__cartesian_fr32")
#pragma file_attr("libFunc=cartesian_fr32")

#pragma file_attr("libName=libdsp")
#pragma file_attr("prefersMem=internal")
#pragma file_attr("prefersMemNum=30")
#endif


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
#endif /* _MISRA_RULES */


#if !defined(TRACE)
#pragma optimize_for_speed
#endif


#if !defined(ETSI_SOURCE)
#define ETSI_SOURCE
#endif /* ETSI_SOURCE */


#include <libetsi.h>
#include <ccblkfn.h>
#include <complex.h>
#include <math.h>

extern fract32
cabs_fr32( complex_fract32 _a )
{
     fract32             a_real, a_imag, cabs32, min_val, max_val;
     unsigned long long  tmp_udiv;


     /* |a.real|, |a.imaginary| */

     a_imag = L_abs(_a.im);
     a_real = L_abs(_a.re);


     if( expected_false( a_real == 0 ) )
     {  
         /* return
              sqrt( 0 + |a.imag|^2 ) = |a.imag| 
         */
         cabs32 = a_imag;  
     }  
     else if( expected_false( a_imag == 0 ) )
     {
         /* return  
              sqrt( |a.real|^2 + 0 ) = |a.real| 
         */
         cabs32 = a_real;
     }
     else if( expected_false( a_imag == a_real ) )
     {
         /* return
              sqrt(a.real^2 + a.real^2) = 
              sqrt(2 * a.real^2) = 
              sqrt(2) * |a.real| = 
              ((sqrt(2)/2) * |a.real|) * 2
         */
         cabs32 = multr_fr1x32x32(0x5a827999, a_real);
         cabs32 = L_add(cabs32, cabs32);
      
     }
     else
     {
         /* Main algorithm */
         min_val = min_fr1x32(a_real, a_imag);
         max_val = max_fr1x32(a_real, a_imag);

         /* cabs32 =  min_val / max_val */     

         tmp_udiv = (((unsigned long long)min_val << 31) / 
                     (unsigned long long) max_val);
         cabs32 =  (fract32) tmp_udiv;

         /* cabs32 = ( min_val^2/max_val^2 ) / 4 ) */
         cabs32 = multr_fr1x32x32(cabs32, cabs32);     
         cabs32 = L_shr(cabs32, 2);

         /* cabs32 = sqrt( cabs32 + 0.25 ) */
         cabs32 = L_add(cabs32,  0x20000000);
         cabs32 = sqrt_fr32(cabs32); 

         /* cabs32 = 2 * cabs32 * max_val */
         cabs32 = multr_fr1x32x32(cabs32, max_val);
         cabs32 = L_add(cabs32, cabs32);
     }      
     return  cabs32;
}

/* End of File */
