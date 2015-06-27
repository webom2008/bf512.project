/* Copyright (C) 2000 Analog Devices, Inc. All Rights Reserved. */
/*
** Default IO device selection. This setting is selected if the user does
** not override it by defining their own version of this symbol.
*/
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libGroup=device.h")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <device.h>

int __default_io_device = PRIMIO;	/* See device.h for alternatives. */
