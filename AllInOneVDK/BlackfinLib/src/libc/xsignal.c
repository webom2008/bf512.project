/*
** Copyright (C) 2001 Analog Devices Inc., All Rights Reserved.
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
** Blackfin vector table for signal() and interrupt().
**/
#pragma file_attr(  "libName=libc")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")

#include <signal.h>
#include "xsignal.h"

vector_entry _vector_table[_NSIG];
