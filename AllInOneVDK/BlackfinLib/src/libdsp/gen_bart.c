// Copyright (C) 2000-2006 Analog Devices Inc., All Rights Reserved.
// This contains Development IP as defined in the ADI/Intel
// Collaboration Agreement (ADI/Intel Confidential)

/******************************************************************************
  Func name   : gen_bartlett_fr16

  Purpose     : Calculate Bartlett Window.
  Description : This function generates a vector containing the Bartlett window.  
                The length is specified by parameter `N`.  
                Note that this window is similar to the Triangle window 
                but has the following different properties: 

                The Bartlett window always returns a window with two zeros 
                on either end of the sequence, so that for odd `n`, the center 
                section of a `N`+2 Bartlett window equals a `N` Triangle window.

                For even n, the Bartlett window is still the convolution of 
                two rectangular sequences.  There is no standard definition for 
                the Triangle window for even `n`; the slopes of the Triangle 
                window are slightly steeper than those of the Bartlett window.

$Revision: 4 $
*******************************************************************************/

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr ("libGroup=window.h")
#pragma file_attr ("libName=libdsp")

#pragma file_attr ("libFunc=gen_bartlett_fr16")
#pragma file_attr ("libFunc=__gen_bartlett_fr16")

#pragma file_attr ("prefersMem=external")
#pragma file_attr ("prefersMemNum=70")
	/* (Use prefersMem=external because the function
	**  is usually called no more than once)
	*/
#endif

#include <window.h>
#include <fract.h>

void
gen_bartlett_fr16(
    fract16 w[],      /* Window array `w` */
    int a,            /* Address stride   */
    int n             /* Window length    */
)
{
    int i,j;
    float c,d,tmp;

    /*{ Check window length and stride }*/
    if( n==1 )
    {
        /*{ window length of 1 would result in 
            division by zero -> return default }*/
        w[0] = 0x0;
        return;
    }
    else if (n > 1 && a > 0)
    {
        /*{ If window length is greater than one and the stride is
            greater than zero, initialize constants and window index }*/
        c = (n - 1.0) / 2.0;
        j = 0;

        /*{ Loop for window length }*/
        for (i = 0; i < n; i++)
        {
            /*{ Calculate Bartlett coefficient }*/
            d = ((float) i - c) / c;
            if (d < (float) 0.0)
                d = -d;
            tmp  = 1.0 - d;
            w[j] = tmp * 32768;
            j += a;
        }

        /* At midpoint (=n/2) the function value will be 1, causing overflow
           => need to set to a valid maximum value
           This is only an issue with arrays of odd length */
        if( n%2==1 )
          w[(n/2)*a] = 0x7fff;
    }
}

/*end of file*/
