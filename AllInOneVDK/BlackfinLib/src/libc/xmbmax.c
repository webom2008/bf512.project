#pragma file_attr("libGroup=locale.h")
#pragma file_attr("libName=libc")
#pragma file_attr("libFunc=_Getmbcurmax")
#pragma file_attr("libFunc=__Getmbcurmax")
#pragma file_attr("prefersMem=any")
#pragma file_attr("prefersMemNum=50")

#include <stdlib.h>

/* For binary compatibility with VisualDSP++5.0 */
size_t _Getmbcurmax() { return MB_CUR_MAX; }
