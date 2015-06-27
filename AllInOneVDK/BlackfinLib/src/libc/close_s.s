/* Copyright (C) 2000-2006 Analog Devices Inc., All Rights Reserved,
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
*/
.file_attr libName=libsmall;
.file_attr FuncName=__Close;
.file_attr libFunc=__Close;
.file_attr libFunc=_Close;
.file_attr libFunc=fclose;
.file_attr libFunc=_fclose;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";

.section program;
.align 2;

__Close:
   JUMP.X _dev_close;
.__Close.end:

.global __Close;
.type __Close,STT_FUNC;
.extern _dev_close;
