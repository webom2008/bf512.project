/* Copyright (C) 2003-2008 Analog Devices Inc., All Rights Reserved,
*/

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.file_attr libName=libc;
.file_attr libGroup=string.h;
.file_attr libFunc=_memchr;
.file_attr FuncName=_memchr;
.file_attr libFunc=memchr;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";
#endif

#include <sys/anomaly_macros_rtl.h>

.section program;

.align 2;

	   //===============================================
	   // C Library function MEMCHR
	   // R0 = address
	   // R1 = sought byte
	   // R2 = count
	   // Returns pointer to located character.
	   //

_memchr:     P2 = R2;            // P2 = count
             P0 = R0 ;           // P0 = address
             R1 = R1.B(Z); 
             CC = R2 == 0;
             IF CC JUMP failed;

             LSETUP (byte_loop_s , byte_loop_e) LC0 = P2;

byte_loop_s:    
#if WA_05000428
                // Speculative read from L2 by Core B
                // may cause a write to L2 to fail

                NOP;
                NOP;
#endif

                R3 = B[P0](Z);   // Get the next character
                CC = R3 == R1;
                IF CC JUMP found;
byte_loop_e:    
                P0+= 1;          // Only increment pointer if byte was not found

failed:      R0 = 0;
             RTS;

found:       R0 = P0;
             RTS;

._memchr.end:

.global _memchr;
.type _memchr,STT_FUNC;
