/*
** Copyright (C) 2003-2009 Analog Devices, Inc. All Rights Reserved.
**
** The default LDFs define a symbol to indicate whether
** L1 Instruction SRAM/Cache has been used to hold code.
** If so, the symbol will have the address 0. Otherwise,
** it will have address 1. If the address is zero,
** the cache init routines will refuse to allow the
** data bank to be used as cache, as that will corrupt
** the code mapped therein. If the LDF does not define
** the symbol at all (because it's not a default LDF),
** then the library definition below will be used instead,
** which will be mapped to a non-zero location, and hence
** will allow the cache area to be used.
** Note: it is the symbol's *address*, not its value, that
** determines the availability of cache; its value is never
** read.
*/

#pragma file_attr(  "libName=libevent")
#pragma file_attr(  "prefersMem=internal")
#pragma file_attr(  "prefersMemNum=30")
int __l1_code_cache;
