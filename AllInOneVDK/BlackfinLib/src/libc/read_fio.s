/* Copyright (C) 2000-2006 Analog Devices Inc., All Rights Reserved,
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
*/
.file_attr libName=libc;
.file_attr FuncName=__Read;
.file_attr libFunc=__Read;
.file_attr libFunc=_Read;
.file_attr libFunc=fread;
.file_attr libFunc=_fread;
.file_attr libFunc=fgets;
.file_attr libFunc=_fgets;
.file_attr libFunc=fgetc;
.file_attr libFunc=_fgetc;
.file_attr prefersMem=external;
.file_attr prefersMemNum="70";

.section program;
.align 2;

__Read:
   [--SP] = R4;
   R4=3;
   EXCPT 5;
   R4 = [SP++];
   RTS;
.__Read.end:

.global __Read;
.type __Read,STT_FUNC;
