/* Copyright (C) 2000-2009 Analog Devices Inc., All Rights Reserved,
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
*/
.file_attr libName=librt;
.file_attr FuncName=__Close;
.file_attr libFunc=__Close;
.file_attr libFunc=_Close;
.file_attr libFunc=fclose;
.file_attr libFunc=_fclose;
.file_attr prefersMem=external;
.file_attr prefersMemNum="70";

#include <sys/anomaly_macros_rtl.h>

.section program;
.align 2;

__Close:
#if WA_05000371
   NOP;
   NOP;
   NOP;
#endif 
   R0=0;
   RTS;
.__Close.end:

.global __Close;
.type __Close,STT_FUNC;
