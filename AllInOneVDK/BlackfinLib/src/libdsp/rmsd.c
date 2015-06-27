/************************************************************************
 *
 * rmsd.c : $Revision: 4 $
 *
 * (c) Copyright 2004 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
 * Description :    Root mean square
 *                    rms = square root( ( sum( a[i] * a[i] ) / n ) )
 */

#pragma file_attr("libGroup =stats.h")
#pragma file_attr("libFunc  =rmsd")
#pragma file_attr("libFunc  =__rmsd")     //from stats.h
#pragma file_attr("libFunc  =rms")        //from stats.h

#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

/* Defined in */
#include <stats.h>

#include <math.h>

long double                       /*{ ret - root mean square         }*/ 
rmsd
( 
  const long double *a,           /*{ (i) - Pointer to input vector  }*/
  int   n                         /*{ (i) - Number of input samples  }*/
)
{
    long double  squaresum = 0.0L;
    int  i;


    /*{ Check the number of element `n`}*/
    if (n <= 0 )
    {
        return 0.0L;
    } 

    /* { Summation of the sqaure of each element of input vector}*/
    for (i = 0; i < n; i++)
    {
        squaresum += (a[i] * a[i]);
    }

    /*{ Average the summation}*/ 
    squaresum /= n;
    
    /*{ Square root the average square value of input vector}*/
    squaresum = sqrtd(squaresum);
    
    return (squaresum);
}
