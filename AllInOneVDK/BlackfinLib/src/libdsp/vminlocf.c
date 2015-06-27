/*************************************************************************
 *
 * vminlocf.c : $Revision: 4 $
 *
 * (c) Copyright 2000-2002 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
   This function finds minimum value index from given float vector
*/

#pragma file_attr("libGroup =vector.h")
#pragma file_attr("libFunc  =vecminlocf")
#pragma file_attr("libFunc  =__vecminlocf") // from vector.h
#pragma file_attr("libFunc  =vecminloc")
#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

#include <vector.h>

int vecminlocf(const float a[], int n)
{
   int min_loc = 0;    /* index of location of minimum no in a vector */
   float min = a[0];
   int i;

   for(i=1; i<n; i++)
   {
      if(a[i]<min)
      {
         min = a[i];
         min_loc = i;
      }
   }
   return min_loc;
}

/* end of file */
