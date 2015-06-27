/************************************************************************
 *
 * xdays_to_mon.c : $Revision: 3542 $
 *
 * (c) Copyright 2002-2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
 * Description :     Internal run time library variables and functions 
 *                   used by time.h
 */
#pragma file_attr(  "libName=libc")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")

#include "xdays_to_mon.h"

/* Days before start of month, for a non-leap year */
const short _days_to_mon[12] = {
    0,  31,  59,  90,
  120, 151, 181, 212,
  243, 273, 304, 334
};

/* Days before start of month, for a leap year */
const short _ldays_to_mon[12] = {
    0,  31,  60,  91,
  121, 152, 182, 213,
  244, 274, 305, 335
};
