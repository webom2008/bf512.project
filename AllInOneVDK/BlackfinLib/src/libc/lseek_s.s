/* Copyright (C) 2001-2006 Analog Devices Inc., All Rights Reserved,
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
*/
.file_attr libName=libsmall;
.file_attr FuncName=__Lseek;
.file_attr libFunc=__Lseek;
.file_attr libFunc=_Lseek;
.file_attr libFunc=fseek;
.file_attr libFunc=_fseek;
.file_attr libFunc=rewind;
.file_attr libFunc=_rewind;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";

.section program;
.align 2;

__Lseek:
   JUMP.X _dev_seek;
.__Lseek.end:

.global __Lseek;
.type __Lseek,STT_FUNC;
.extern _dev_seek;
