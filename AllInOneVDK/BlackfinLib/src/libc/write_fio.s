/* Copyright (C) 2000-2006 Analog Devices Inc., All Rights Reserved,
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
*/
.file_attr libName=libc;
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

.section program;
.align 2;

__Write:
   [--SP] = R4;
   R4=2;
   EXCPT 5;
   R4 = [SP++];
   RTS;
.__Write.end:

.global __Write;
.type __Write,STT_FUNC;
