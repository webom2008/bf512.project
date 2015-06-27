/************************************************************************
 *
 * alogf.c : $Revision: 4 $
 *
 * (c) Copyright 2003 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
 * Description    : This file contains the implementation of alogf()
 */

#include <math.h>

/*____________________________________________________________________________

  Func name   : alogf

  ----------------------------------------------------------------------------

  Purpose     : Natural anti-log
  Description : This function calculates the natural (base e) anti-log of
                its argument.

  Domain      : as for expf

  Notes       : An anti-log function performs the reverse of a log function
                and is therefore equivalent to an exponentation operation.
  ____________________________________________________________________________
*/

#pragma file_attr("libGroup =math.h")
#pragma file_attr("libGroup =math_bf.h")
#pragma file_attr("libFunc  =__alogf")
#pragma file_attr("libFunc  =alogf")
#pragma file_attr("libFunc  =alog")
#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

float
alogf(float x)
{

   return expf(x);

}
