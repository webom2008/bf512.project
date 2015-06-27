/* Copyright (C) 2006 Analog Devices, Inc. All Rights Reserved.
**
** A wrapper around the cli() compiler built-in. Only used if you
** attempt to call the cli() built-in without including the
** <ccblkfn.h> header in your C source, or you call it directly
** from assembly.
*/


int cli(void) {
  int r;
#ifdef __AVOID_CLI_ANOMALY__
  int reti;
  __asm volatile("%0 = RETI; RETI = [SP++];\n" : "=d" (reti));
#endif
  r = __builtin_cli();
#ifdef __AVOID_CLI_ANOMALY__
  __asm volatile("[--SP] = RETI; RETI = %0;\n" : : "d" (reti) : "reti");
#endif
   return r;
}
