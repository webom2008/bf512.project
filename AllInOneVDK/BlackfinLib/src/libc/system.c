/* Copyright (C) 2000 Analog Devices Inc., All Rights Reserved,
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
*/
/*
** Stub for system(), which isn't supported.
*/
#pragma file_attr(  "libName=libc")
#pragma file_attr(  "libFunc=system")
#pragma file_attr(  "libFunc=_system")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")

int system(const char *string)
{
  /* If string==NULL, then the caller is enquiring whether a commmand
     processor is available. Zero means no, non-zero means yes.
     If string!=NULL, it's a command, and the result is implementation-
     defined. */
  return 0;
}
