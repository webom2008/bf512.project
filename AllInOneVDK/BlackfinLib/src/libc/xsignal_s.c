/*
** Copyright (C) 2001 Analog Devices Inc., All Rights Reserved.
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
** Blackfin vector table for signal() and interrupt().
**/
#pragma file_attr(  "libName=libsmall")
#pragma file_attr(  "prefersMem=internal")
#pragma file_attr(  "prefersMemNum=30")

#include <signal.h>
#include "xsignal_s.h"

_Sigfun * _vector_table[_NSIG];
