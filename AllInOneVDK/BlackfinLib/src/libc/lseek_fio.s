/* Copyright (C) 2001-2006 Analog Devices Inc., All Rights Reserved,
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
*/
.file_attr libName=libc;
.file_attr FuncName=__Lseek;
.file_attr libFunc=__Lseek;
.file_attr libFunc=_Lseek;
.file_attr libFunc=fseek;
.file_attr libFunc=_fseek;
.file_attr libFunc=rewind;
.file_attr libFunc=_rewind;
.file_attr prefersMem=external;
.file_attr prefersMemNum="70";

.section program;
.align 2;

__Lseek:
   [--SP] = R4;
   R4=4;
   EXCPT 5;
   R4 = [SP++];
   RTS;
.__Lseek.end:

.global __Lseek;
.type __Lseek,STT_FUNC;
