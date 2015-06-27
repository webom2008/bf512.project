/************************************************************************
 *
 * _primio_intrpt.h : $Revision: 4 $
 *
 * (c) Copyright 2002-2003 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

 /* Macros for enabling/disabling interrupts on Blackfin */

typedef int interrupt_state;

#define DISABLE_INTERRUPTS(S) (S) = __builtin_cli()
#define ENABLE_INTERRUPTS(S)  __builtin_sti((S))

// end of file
