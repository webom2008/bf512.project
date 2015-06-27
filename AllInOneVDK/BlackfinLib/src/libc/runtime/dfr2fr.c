/* Copyright (C) 2000 Analog Devices Inc., All Rights Reserved.
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
*/
/*
 * Convert a fract32 to a fract16.
 */
#pragma file_attr(  "libName=librt")
#pragma file_attr(  "libName=librt_fileio")
#pragma file_attr(  "libFunc=fr32_to_fr16")
#pragma file_attr(  "libFunc=_fr32_to_fr16")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")

#include <fract_typedef.h>

fract16
fr32_to_fr16(fract32 val)
{
	return (fract16)(val >> 16); 
} 
