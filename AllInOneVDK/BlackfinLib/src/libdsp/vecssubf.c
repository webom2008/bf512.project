// Copyright (C) 2000 Analog Devices Inc., All Rights Reserved.
// This contains Development IP as defined in the ADI/Intel 
// Collaboration Agreement (ADI/Intel Confidential)



/*{!CFH}______________________________________________________________________

  Func name   : vecssubf

  ----------------------------------------------------------------------------

  Purpose     : Real vector - scalar subtraction for float data types.
  Description : This function subtracts input scalar `b` from each element of 
                input vector `a[]` and stores the result in vector `c[]`.

  Domain      : Full Range.

  Accuracy    : 0 bits in error.

  Data Memory : 0 words.
  Prog Memory : 13 words.
  Cycles      : ~ (n <= 0): 17 cycles.
                ~ (n > 0): 14 + (4 * n) cycles.
                ~ Where `n` is the size of the input data array.

  Notes       : Output can be written to input vector.
  _____________________________________________________________________{!EHDR}
*/
#include <vector.h>

#pragma file_attr("libGroup =vector.h")
#pragma file_attr("libGroup =matrix.h")
#pragma file_attr("libFunc  =vecssubf")
#pragma file_attr("libFunc  =__vecssubf")
#pragma file_attr("libFunc  =vecssub")      // from vector.h
#pragma file_attr("libFunc  =matssubf")     // from matrix.h
#pragma file_attr("libFunc  =matssub")      // from matrix.h
#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

void
_vecssubf(
	const float a[],  /*{ (i) - Input vector `a[]` }*/
	float b,          /*{ (i) - Input scalar `b`}*/
	float c[],        /*{ (o) - Output vector `c[]` }*/
	int n             /*{ (i) - Number of elements in vector }*/
)
{
	int i;

    /*{ Subtract `b` from each element of vector `a[]` and store
        in vector `c[]`. }*/
    for (i = 0; i < n; i++)
	    c[i] = a[i] - b;
}
/*end of file*/
