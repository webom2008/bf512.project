/* Copyright (C) 2006-2009 Analog Devices, Inc. All Rights Reserved.
**
** An out-of-line version of the adi_core_b_enable() routine.
** If you include <ccblkfn.h> in your C source, you will have
** an inline version, which is recommended.
*/

#if __NUM_CORES__ > 1

 #if defined(__ADSPBF561__)
 #include <cdefBF561.h>

void adi_core_b_enable(void)
{
  /* Clearing bit 5 allows core B to run. */
  *pSICA_SYSCR &= ~(1<<5);
  /* Setting it again releases it from its waiting loop in the CRT. */
  *pSICA_SYSCR |= 1<<5;
}

 #else
  #error adi_core_b_enable not implemented for target.
 #endif

#else

void adi_core_b_enable(void)
{
  /* Do nothing. */
}

#endif
