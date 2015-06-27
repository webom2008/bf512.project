/* Copyright (C) 2006-2009 Analog Devices, Inc. All Rights Reserved.
**
** An out-of-line version of the adi_core_id() routine.
** If you include <ccblkfn.h> in your C source, you will have
** an inline version, which is recommended.
*/

#if __NUM_CORES__ > 1

 #if defined(__ADSPBF561__)
 #include <cdefBF561.h>

int adi_core_id(void) {
  /* Return the core ID: 0 for core A, 1 for core B.
   * (The SRAM base is 0xFF800000 for core A and 0xFF400000 for core B.)
   */
  unsigned int sram_base = *(volatile unsigned int *)SRAM_BASE_ADDRESS;
  int id = (sram_base & 0x400000) != 0;
  return id;
}

 #else
  #error adi_core_id not implemented for target.
 #endif

#else

int adi_core_id(void) {
  /* NULL version, for when the Core-A part of an application
  ** is run on a single-core system.
  */
  return 0;
}

#endif /* defined(__ADSPBF561__) */
