// Copyright (C) 2004 Analog Devices Inc., All Rights Reserved.
/******************************************************************************
  Func name   : llclip

  Purpose     : Clip value a if it exceeds the absolute maximum boundary. 
                
  Domain      : ~ a = [LLONG_MIN ... LLONG_MAX]

******************************************************************************/

#pragma file_attr("libGroup =math_bf.h")
#pragma file_attr("libGroup =math.h")
#pragma file_attr("libFunc  =__llclip")
#pragma file_attr("libFunc  =llclip")
#pragma file_attr("libName =libdsp")
#pragma file_attr("prefersMem =internal")
#pragma file_attr("prefersMemNum =30")

#include <math.h>

long long  _llclip( long long  a, long long  max_val ) 
{
      long long   abs_max_val;

      abs_max_val = llabs(max_val);
      if( llabs(a) < abs_max_val )
      {
          return a;
      }
      else
      {
          return ( a < 0 ? -abs_max_val : abs_max_val );
      }
}

/*end of file*/
