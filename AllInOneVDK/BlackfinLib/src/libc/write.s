/* Copyright (C) 2000-2009 Analog Devices Inc., All Rights Reserved,
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
*/
.file_attr libName=librt;
.file_attr FuncName=__Write;
.file_attr libFunc=__Write;
.file_attr libFunc=_Write;
.file_attr libFunc=fwrite;
.file_attr libFunc=_fwrite;
.file_attr libFunc=fprintf;
.file_attr libFunc=_fprintf;
.file_attr libFunc=fputs;
.file_attr libFunc=_fputs;
.file_attr libFunc=fputc;
.file_attr libFunc=_fputc;
.file_attr prefersMem=external;
.file_attr prefersMemNum="70";

#include <sys/anomaly_macros_rtl.h>

.section program;
.align 2;

__Write:
#if WA_05000371
   NOP;
   NOP;
   NOP;
#endif 
   R0 = 0;
   RTS;
.__Write.end:

.global __Write;
.type __Write,STT_FUNC;
