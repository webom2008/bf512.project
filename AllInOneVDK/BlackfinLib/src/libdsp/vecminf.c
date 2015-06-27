// Copyright (C) 2000, 2001 Analog Devices Inc., All Rights Reserved.
// This contains Development IP as defined in the ADI/Intel
// Collaboration Agreement (ADI/Intel Confidential)

/****************************************************************************
   File: vecminf.c
 
   Returns minimum value stored in input vector a
    
***************************************************************************/

#pragma file_attr("libGroup =vector.h")
#pragma file_attr("libFunc  =vecminf")
#pragma file_attr("libFunc  =__vecminf")
#pragma file_attr("libFunc  =vecmin")      // from vector.h
#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

#include <vector.h>

float _vecminf(const float a[],int n)
{
	float min;
	int i;

	if( n <= 0 )
		return 0.0;

	min = a[0];
	for(i=1;i<n;i++)
	{
		if(a[i]<min)
		{
			min = a[i];
		}
	}

        return min;
}

/*end of file*/
