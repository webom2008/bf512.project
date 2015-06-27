/* Copyright (C) 2000-2006 Analog Devices Inc., All Rights Reserved,
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
*/
.file_attr libName=libc;
.file_attr FuncName=__Open;
.file_attr libFunc=__Open;
.file_attr libFunc=_Open;
.file_attr libFunc=fopen;
.file_attr libFunc=_fopen;
.file_attr prefersMem=external;
.file_attr prefersMemNum="70";

.section program;
.align 2;

__Open:
   [--SP] = R4;
   R4=0;
   EXCPT 5;
   R4 = [SP++];
   RTS;
.__Open.end:
		
.global __Open;
.type __Open,STT_FUNC;
