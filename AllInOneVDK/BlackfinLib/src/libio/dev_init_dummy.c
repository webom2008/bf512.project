/************************************************************************
 *
 * dev_init_dummy.c : $Revision: 1.4 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 * Dummy implementation of init_devtab(), which might still be
 * called from old Blackfin CRTs, but which has been replaced with
 * on-demand initialisation though __init_devtabs().
 *
 ************************************************************************/
 
#ifndef __ADSPBLACKFIN__
#error This should be built for Blackfin only.
#endif

void init_devtab(void) {}
