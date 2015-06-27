/* Copyright (C) 2000-2008 Analog Devices Inc., All Rights Reserved,
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
*/
/*
 * Fast strcmp() for Blackfin.
 * When both strings are aligned, this processes four characters at
 * a time. Uses a hw loop with "very big" count to loop "forever",
 * until difference or a terminating zero is found.
 * Once the end-case word has been identified, breaks out of the
 * loop to check more carefully (same as the unaligned case).
 */

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.file_attr libName=libc;
.file_attr libGroup=string.h;
.file_attr FuncName=_strcmp;
.file_attr libFunc=_strcmp;
.file_attr libFunc=strcmp;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";
#endif

#include <sys/anomaly_macros_rtl.h>

.section program;

.align 2;

_strcmp:
	[--sp] = (R7:6);
	p1 = r0;
	p2 = r1;

	p0 = -1;		// (need for loop counter init)

	 // check if byte aligned
	r0 = r0 | r1;		// check both pointers at same time
	r0 <<= 30;		// dump all but last 2 bits
	cc = az;		// are they zero?
	if !cc jump unaligned;	// no; use unaligned code.
				// fall-thru for aligned case..

	  // note that r0 is zero from the previous...
	  //           p0 set to -1

	lsetup (beginloop, endloop) lc0=p0;
	  // make up mask:  0FF0FF
	r7 = 0xFF;
	r7.h = 0xFF;
	  // pick up first words
	r1 = [p1++];
	r2 = [p2++];
	  // loop : 9 cycles to check 4 characters
	cc = r1 == r2;
beginloop:
	if !cc jump notequal4;	// compare failure, exit loop

	  // starting with   44332211
	  // see if char 3 or char 1 is 0
	r2 = r1 & r7;		// form 00330011
	r3 = r1 ^ r2;		// form 44002200 (4321^0301 => 4020)
				// (trick, saves having another mask)
#if WA_05000428
	// Speculative read from L2 by Core B may cause a write to L2 to fail

	nop;
#endif

	r6 = r2 +|+ r0 ||	// add to zero
	  r2 = [p2++] ||	// (r2 is free, reload)
	  nop;
	cc = az;		// true if either is zero
	// add to zero , and  (r1 is free, reload)
	r6 = r3 +|+ r0 || r1 = [p1++] || nop;
	cc |= az;		// true if either is zero
	if cc jump zero4;	// leave if a zero somewhere
endloop:
	cc = r1 == r2;

 // loop exits
notequal4:		// compare failure on 4-char compare
			// address pointers are one word ahead;
			// faster to use zero4 exit code
	p1 += 4;
	p2 += 4;

zero4:			// one of the bytes in word 1 is zero
			// but we've already fetched the next word; so
			// backup two to look at failing word again
	p1 += -8;
	p2 += -8;



		// here when pointers are unaligned: checks one
		// character at a time.  Also use at the end of
		// the word-check algorithm to figure out what happened
unaligned:
	  //	R0 is non-zero from before.
	  //           p0 set to -1

	r0 = 0 (Z);
	r1 = B[p1++] (Z);
	r2 = B[p2++] (Z);
	lsetup (beginloop1, endloop1) lc0=p0;

beginloop1:
	cc = r1;		// first char must be non-zero
	r3 = r2 - r1 (NS) ||	// chars must be the same
	  r1 = B[p1++] (Z) ||
	  nop;
	cc &= az;
	r3 = r0 - r2;		// second char must be non-zero
	cc &= an;

#if WA_05000428
	// Speculative read from L2 by Core B may cause a write to L2 to fail
	//
	// Avoid the anomaly by loading the next character via P2 _before_
	// executing a conditional jump - this means that once outside the
	// loop, the original character in the [P2] string will have to be
	// re-loaded

	r2 = B[p2++] (Z);
endloop1:
	if !cc jump exitloop1;
#else
	if !cc jump exitloop1;
endloop1:
	r2 = B[p2++] (Z);
#endif



exitloop1: // here means we found a zero or a difference.
	   // we have r2(N), p2(N), r1(N+1), p1(N+2)
#if WA_05000428
	// Speculative read from L2 by Core B may cause a write to L2 to fail

	nop;
	nop;
	nop;
	r2=B[p2+ -2] (Z);
#endif

	r1=B[p1+ -2] (Z);
	r0 = r1 - r2;
	(r7:6) = [sp++];
	rts;
._strcmp.end:
.type _strcmp, STT_FUNC;
.global _strcmp;
