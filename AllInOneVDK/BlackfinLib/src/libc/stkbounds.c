/************************************************************************
 *
 * stkbounds.c: $Revision: 1.2.10.1 $
 *
 * (c) Copyright 2009-2010 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/
#pragma file_attr("libGroup=stack_overflow_detection")
#pragma file_attr("libName=librt")
#pragma file_attr("libName=librt_filio")
#pragma file_attr("prefersMem=any")
#pragma file_attr("prefersMemNum=50")
#pragma file_attr("ThreadPerCoreSharing=MustShare")

/* The __adi_stack_bounds array records the stack limit on each core in
 * a DXE. On single-cores, this array always has only one element.
 * On dualcores, it depends on which programming model is used.
 * In the app-per-core and the deprecated five-project models, each core
 * has its own DXE with a one-element array.
 * In the thread-per-core model, one DXE covers both cores, hence the
 * array has two elements. In this case, the compiler generates code to
 * index the array by core ID when doing stack bounds checking.
 */

extern "asm" void *ldf_stack_space;

#if !defined(_LIBTPC)

/* Single-core and app-per-core. */
volatile void *__adi_stack_bounds[1] = { &ldf_stack_space };

#elif __NUM_CORES__ == 2

/* Thread-per-core */
extern "asm" void *CoreB_ldf_stack_space;

volatile void *__adi_stack_bounds[__NUM_CORES__] = {
  &ldf_stack_space,
  &CoreB_ldf_stack_space
};

#else

#error Not configured for number of cores on processor.

#endif
