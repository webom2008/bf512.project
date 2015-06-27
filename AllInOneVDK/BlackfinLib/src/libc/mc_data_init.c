/****************************************************************************
 *
 * mc_data_init.c : $Revision: 1.2 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 * Empty implementation of _mc_data_initialise routine.
 * Required for backward compatibility with <=5.0 BF561 CRTs.
 *
 ****************************************************************************/

#ifndef __ADSPBF561__
#error This should be built for the BF561 only.
#endif

#pragma file_attr( "libName=libc")
#pragma file_attr( "libFunc=_mc_data_initialise")
#pragma file_attr(  "libFunc=__mc_data_initialise")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

void _mc_data_initialise(void) {}
