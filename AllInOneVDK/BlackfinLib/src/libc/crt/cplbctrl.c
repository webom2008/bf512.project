/* Copyright (C) 2010 Analog Devices, Inc. All Rights Reserved. */
/*
** CPLB and cache configuration are defined by this variable.
** Users can edit it or replace it with their own definition
** during linking, to change how the system is set up.
*/
#pragma file_attr(  "libName=libevent")
#pragma file_attr(  "prefersMem=internal")
#pragma file_attr(  "prefersMemNum=30")

#include "cplb.h"

#if !defined(_LIBTPC)
int __cplb_ctrl = 0;
#else
int __CoreB_cplb_ctrl = 0;
#endif
