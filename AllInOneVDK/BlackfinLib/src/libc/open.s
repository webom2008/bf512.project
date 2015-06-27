/* Copyright (C) 2000-2009 Analog Devices Inc., All Rights Reserved,
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
*/
.file_attr libName=librt;
.file_attr FuncName=__Open;
.file_attr libFunc=__Open;
.file_attr libFunc=_Open;
.file_attr libFunc=fopen;
.file_attr libFunc=_fopen;
.file_attr prefersMem=external;
.file_attr prefersMemNum="70";

#include <sys/anomaly_macros_rtl.h>

.section program;
.align 2;

__Open:
#if WA_05000371
   NOP;
   NOP;
   NOP;
#endif 
   R0=-1;
   RTS;
.__Open.end:

.global __Open;
.type __Open,STT_FUNC;
