// Copyright (C) 2000-2006 Analog Devices Inc., All Rights Reserved.
// This contains Development IP as defined in the ADI/Intel
// Collaboration Agreement (ADI/Intel Confidential)

/******************************************************************************
  Func name   : gen_hanning_fr16

  Purpose     : Calculate Hanning Window.
  Description : This function generates a vector containing the Hanning window. 
                The length is specified by parameter `N`.

$Revision: 4 $
*******************************************************************************/

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr ("libGroup=window.h")
#pragma file_attr ("libName=libdsp")

#pragma file_attr ("libFunc=gen_hanning_fr16")
#pragma file_attr ("libFunc=__gen_hanning_fr16")

#pragma file_attr ("prefersMem=external")
#pragma file_attr ("prefersMemNum=70")
	/* (Use prefersMem=external because the function
	**  is usually called no more than once)
	*/
#endif

#define  ETSI_SOURCE
#include <math.h>
#include <window.h>
#include <fract.h>
#include <fract_math.h>
#include "Lib_cos16_2PIx.h"

void
gen_hanning_fr16(
    fract16 w[],      /* Window array `w` */
    int a,            /* Address stride */
    int n             /* Window length */
)
{
    int    i, scaled_i, n_half, n_offset;
    float  inv_n;

    /* Check window length and stride */
    if( n == 1 )
    {    
        /* window length of 1 would result in division by zero -> return default */
        w[0] = 0x0;
        return;
    }
    else if ( n > 0 && a > 0 )
    {
         /* If window length is greater than zero and the stride is
            greater than zero, initialize constants and window index */
        n_half = n/2;
        n--;
        n_offset = n * a;
        inv_n = 1.0 / (float) n;

        /* Hanning / Hann / Von Hann window:
                       w[j] = 0.5( 1.0 - cos( 2pi * ( i / (n-1) ))
                                where i = 0,1,..,n-1,
                                      j = 0,a,..,a*(n-1)
                                coefficient in fract16: 0.50 = 0x4000        */

        /*  First element: 0.5( 1.0 - cos(0) ) = 0  */
        w[0] = 0x0;
        scaled_i =  a;

        /* Loop for window length */
        /*  First and last element computed outside the loop ! */
        for (i = 1; i < n_half; i++)
        {
            /* Calculate Hanning coefficient */
            w[scaled_i] = sub( 0x4000,
                               ( __cos16_2PIx( i * inv_n) >> 1 ));

            /* function is symetric => fill array from far end 
                index: w[ ( ((n-1)-i) * a) ] = w[ (i * a) ]  */
            w[n_offset - scaled_i] = w[scaled_i];
           
            scaled_i += a;
        }

        /*  Last element: 0.5( 1.0 - cos(2pi) ) = 0  */
        w[n_offset] = 0x0;

        /* At midpoint (=n/2) the function value will be 1, causing overflow
           => need to set to a valid maximum value
           This is only an issue with arrays of odd length */
        n++;
        if( n&1 !=0 )
            w[n_half * a] = 0x7fff;
    }
}

/*end of file*/
