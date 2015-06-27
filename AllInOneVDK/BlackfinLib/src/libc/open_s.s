/* Copyright (C) 2000-2006 Analog Devices Inc., All Rights Reserved,
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
*/
.file_attr libName=libsmall;
.file_attr FuncName=__Open;
.file_attr libFunc=__Open;
.file_attr libFunc=_Open;
.file_attr libFunc=fopen;
.file_attr libFunc=_fopen;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";

.section program;
.align 2;

__Open:
  P1.H = ___default_io_device;
  P1.L = ___default_io_device;
  R2 = R1;
  R1 = R0;
  R0 = [P1];
  JUMP.X _dev_open;
.__Open.end:
		
.global __Open;
.type __Open,STT_FUNC;
.extern _dev_open;
.extern ___default_io_device;
