/* Copyright (C) 2000-2006 Analog Devices Inc., All Rights Reserved,
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
*/
.file_attr libName=libc;
.file_attr FuncName=__Close;
.file_attr libFunc=__Close;
.file_attr libFunc=_Close;
.file_attr libFunc=fclose;
.file_attr libFunc=_fclose;
.file_attr prefersMem=external;
.file_attr prefersMemNum="70";

.section program;
.align 2;

__Close:
   [--SP] = R4;
   R4=1;
   EXCPT 5;
   R4 = [SP++];
   RTS;
.__Close.end:

.global __Close;
.type __Close,STT_FUNC;
