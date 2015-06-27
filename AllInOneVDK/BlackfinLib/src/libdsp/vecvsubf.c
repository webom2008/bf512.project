// Copyright (C) 2000 Analog Devices Inc., All Rights Reserved.
// This contains Development IP as defined in the ADI/Intel 
// Collaboration Agreement (ADI/Intel Confidential)


/*}______________________________________________________________________

  Func name   : vecvsubf

  ----------------------------------------------------------------------------

  Purpose     : Real vector - vector subtraction for float data types.
  Description : This function subtracts each element of vector `b[]` from 
                each element of input vector `a[]` and stores the result in 
                vector `c[]`.

  Domain      : Full Range.

  Accuracy    : 0 bits in error.
  _____________________________________________________________________
*/

#pragma file_attr("libGroup =vector.h")
#pragma file_attr("libGroup =matrix.h")
#pragma file_attr("libFunc  =vecvsubf")    // from vector.h
#pragma file_attr("libFunc  =__vecvsubf")  
#pragma file_attr("libFunc  =matmsubf")    // from matrix.h
#pragma file_attr("libFunc  =matmsub")     // from matrix.h
#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

#include <vector.h>

void
_vecvsubf(
    const float a[],    /*{ (i) - Input vector `a[]` }*/
    const float b[],    /*{ (i) - Input vector `b[]` }*/
    float c[],          /*{ (o) - Output vector `c[]` }*/
    int n               /*{ (i) - Number of elements in vector }*/
)
{
    int i;

    /*{ Each element of vector `b[]` is subtracted from each element
        of vector `a[]` and stored in vector `c[]`. }*/
    for (i = 0; i < n; i++)
        c[i] = a[i] - b[i];
}

/*end of file*/
