/************************************************************************
 *
 * proccycles.c : $Revision: 4 $
 *
 * (c) Copyright 2006 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/* Define _Processor_cycles_per_sec needed by CLOCKS_PER_SEC */

#pragma file_attr("libName=libc")
#pragma file_attr("prefersMem=any")
#pragma file_attr("prefersMemNum=50")
volatile int _Processor_cycles_per_sec = -1;
