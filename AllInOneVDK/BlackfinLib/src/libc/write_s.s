/* Copyright (C) 2000-2006 Analog Devices Inc., All Rights Reserved,
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
**
** Called as part of the I/O system to write some data to a device
*/
.file_attr libName=libsmall;
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
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";

.section program;
.align 2;

__Write:
   JUMP.X _dev_write;
.__Write.end:

.global __Write;
.type __Write,STT_FUNC;
.extern _dev_write;
