/* Copyright (C) 2000 Analog Devices Inc., All Rights Reserved.
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
*/
/*
 * Convert a fract16 to a fract32.
 */
#pragma file_attr(  "libName=librt")
#pragma file_attr(  "libName=librt_fileio")
#pragma file_attr(  "libFunc=fr16_to_fr32")
#pragma file_attr(  "libFunc=_fr16_to_fr32")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")

#include <fract_typedef.h>

fract32
fr16_to_fr32(fract16 val)
{
	return ((fract32)(val)) << 16; 
}
