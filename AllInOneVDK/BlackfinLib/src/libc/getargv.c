/* Copyright (C) 2000-2003 Analog Devices Inc., All Rights Reserved,
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
*/

#pragma file_attr( "libName=librt_fileio")
#pragma file_attr(  "libFunc=_getargv")
#pragma file_attr(  "libFunc=__getargv")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")
extern char __argv_string[];

char *_Argv[20];

int _getargv(void)
{
  int argc;
  volatile char *p;
  p = __argv_string;
  argc=0;
  if (*p) {
    _Argv[argc++] = (char *)p;
	 while (*p) {
      if (*p==' ') {
        *p++ = 0; 
        _Argv[argc++] = (char *)p;
      }
      p++;
    }
  }
  return argc;
}

