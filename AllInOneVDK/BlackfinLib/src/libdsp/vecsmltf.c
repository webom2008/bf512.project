// Copyright (C) 2000 Analog Devices Inc., All Rights Reserved.
// This contains Development IP as defined in the ADI/Intel 
// Collaboration Agreement (ADI/Intel Confidential)


/*______________________________________________________________________

  Func name   : vecsmltf

  ----------------------------------------------------------------------------

  Purpose     : Real vector * scalar multiplication for float data types.
  Description : This function multiplies input scalar `b` with each element of 
                input vector `a[]` and stores the result in vector `c[]`.

  Domain      : Full Range.

  Accuracy    : 0 bits in error.

  Data Memory : 0 words.
  Prog Memory : 13 words.
  Cycles      : ~ (n <= 0): 17 cycles.
                ~ (n > 0): 14 + (4 * n) cycles.
                ~ Where `n` is the size of the input data array.

  Notes       : Output can be written to input vector.
  _____________________________________________________________________
*/

#pragma file_attr("libGroup =vector.h")
#pragma file_attr("libGroup =matrix.h")
#pragma file_attr("libFunc  =vecsmltf")
#pragma file_attr("libFunc  =__vecsmltf")
#pragma file_attr("libFunc  =vecsmlt")      // from vector.h
#pragma file_attr("libFunc  =matsmltf")     // from matrix.h
#pragma file_attr("libFunc  =matsmlt")      // from matrix.h
#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

#include <vector.h>

void
_vecsmltf(
	const float a[],    /*{ (i) - Input vector `a[]` }*/
	float b, 	        /*{ (i) - Input scalar `b` }*/
	float c[], 	        /*{ (o) - Output vector `c[]` }*/
	int n		        /*{ (i) - Number of elements in vector }*/
)
{
	int i;

    /*{ Multiply `b` with each element of vector `a[]` and store
        in vector `c[]`. }*/
    for (i = 0; i < n; i++)
	    c[i] = a[i] * b;
}

/*end of file*/
