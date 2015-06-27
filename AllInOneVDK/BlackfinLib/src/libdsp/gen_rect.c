// Copyright (C) 2000-2006 Analog Devices Inc., All Rights Reserved.
// This contains Development IP as defined in the ADI/Intel
// Collaboration Agreement (ADI/Intel Confidential)


/******************************************************************************
  Func name   : gen_rectangular_fr16
  Purpose     : Calculate rectangular Window.
  Description : This function generates a vector containing the rectangular 
                window.  The length is specified by parameter `N`.

$Revision: 4 $
*******************************************************************************/

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr ("libGroup=window.h")
#pragma file_attr ("libName=libdsp")

#pragma file_attr ("libFunc=gen_rectangular_fr16")
#pragma file_attr ("libFunc=__gen_rectangular_fr16")

#pragma file_attr ("prefersMem=external")
#pragma file_attr ("prefersMemNum=70")
	/* (Use prefersMem=external because the function
	**  is usually called no more than once)
	*/
#endif

#include <window.h>
#include <fract.h>

void
gen_rectangular_fr16(
    fract16 w[],       /* Window array `w`*/
    int a,             /* Address stride  */
    int n              /* Window length   */
)
{
    int i, j;

    /* Check window length and stride */
    if (n > 0 && a > 0)
    {
        /* If window length is greater than zero and the stride is
           greater than zero, initialize constants and window index */
        j = 0;

        /*{ Loop for window length }*/
        for (i = 0; i < n; i++)
        {
           /*{ Calculate rectangular coefficient }*/
           w[j] = 0x7fff;
           j += a;
        }
    }
}

/* end of file */
