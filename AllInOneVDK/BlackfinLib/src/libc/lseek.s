/* Copyright (C) 2001-2009 Analog Devices Inc., All Rights Reserved,
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
*/
.file_attr libName=librt;
.file_attr FuncName=__Lseek;
.file_attr libFunc=__Lseek;
.file_attr libFunc=_Lseek;
.file_attr libFunc=fseek;
.file_attr libFunc=_fseek;
.file_attr libFunc=rewind;
.file_attr libFunc=_rewind;
.file_attr prefersMem=external;
.file_attr prefersMemNum="70";

#include <sys/anomaly_macros_rtl.h>

.section program;
.align 2;

__Lseek:
#if WA_05000371
   NOP;
   NOP;
   NOP;
#endif 
   R0= -1 (X);
   RTS;
.__Lseek.end:

.global __Lseek;
.type __Lseek,STT_FUNC;
