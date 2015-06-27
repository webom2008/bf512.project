/* Copyright (C) 2000-2006 Analog Devices Inc., All Rights Reserved,
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
*/
.file_attr libName=libsmall;
.file_attr FuncName=__Read;
.file_attr libFunc=__Read;
.file_attr libFunc=_Read;
.file_attr libFunc=fread;
.file_attr libFunc=_fread;
.file_attr libFunc=fgets;
.file_attr libFunc=_fgets;
.file_attr libFunc=fgetc;
.file_attr libFunc=_fgetc;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";

.section program;
.align 2;

__Read:
   JUMP.X _dev_read;
.__Read.end:

.global __Read;
.type __Read,STT_FUNC;
.extern _dev_read;
