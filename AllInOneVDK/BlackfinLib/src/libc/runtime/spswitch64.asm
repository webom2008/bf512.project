/*****************************************************************************
 *
 * spswitch64.asm : $Revision: 1.6 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 *****************************************************************************/

#if defined(__SYSCALL_INFO__)
/* System call information */
%regs_clobbered "R2-R3,P0-P2,LC0,LT0,LB0,CCset"
%notes "Compiler support routine for 64-bit sparse switch statements."
%const
%syscall ___spswitch64(V=lLpV)
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

.FILE_ATTR FuncName      = ___spswitch64;
.FILE_ATTR libName       = libc;

.FILE_ATTR libGroup      = support;
.FILE_ATTR libFunc       = ___spswitch64;

.FILE_ATTR prefersMem    = any;
.FILE_ATTR prefersMemNum = "50";

#endif


.SECTION/DOUBLEANY program;
.ALIGN 2;

   //===============================================
   // ___spswitch64: runtime support for sparse switches
   //
   //   R0 = val_in.lo
   //   R1 = val_in.hi
   //   P0 = (struct { int N; uint64 values[N]; Label default, labels[N]; }*)
   //   clobbers R0,R1,R2,R3,P0,P1,P2

___spswitch64:
      [--SP] = (P5:3);
      P1 = [P0++];               // P1 = N, P0 = &values
      P2 = 0;                    // lo
      P3 = P1 << 1;
      P4 = P0 + (P3 << 2);
      P4 += 4;                   // labels = &values + N*sizeof(long long) + 4

      LOOP (.binary_search) LC0 = P1; // will finish before N iterations
      LOOP_BEGIN .binary_search;
         // start of each iteration
         //   P1 = hi, P2 = lo, P0 = values, R0 = val_in
         //   we know  values[hi] > val_in and values[lo-1] < val_in

         CC = P1 <= P2;
         IF CC JUMP .not_found;  // if (hi <= lo) not found

         P3 = P1 + P2;
         P3 = P3 >> 1;           // mid = (hi + lo) / 2
         P5 = P3 << 1;
         P5 = P0 + (P5 << 2);    // &values[mid]

         R2 = [P5++];            // this_val.lo = values[mid].lo
         R3 = [P5];              // this_val.hi = values[mid].hi

         CC = R1 == R3;          // if (val_in.hi == this_val.hi)
         IF CC R3 = R2;          //   R2 = val_in.lo, R3 = this_val.lo
         IF CC R2 = R0;          // else
         IF !CC R2 = R1;         //   R2 = val_in.hi, R3 = this_val.hi
         
         CC = R2 == R3;
         IF CC JUMP .found;      // if (this_val == val_in) found
         
         CC = R3 < R2 (IU);
         IF !CC P1 = P3;         // if (this_val < val_in) hi = mid
         P3 += 1;
         IF CC P2 = P3;          // if (this_val > val_in) lo = mid + 1
      LOOP_END .binary_search;

.not_found:
      P3 = -1;                   // mid = -1, index of default lab
.found:
      // LC0 = 0; ???
      P1 = P4 + (P3 << 2);
      P1 = [P1];                 // lab = labels[mid]
      (P5:3) = [SP++];
      JUMP (P1);                 // goto lab;

.___spswitch64.end:

        .GLOBAL ___spswitch64;
        .TYPE ___spswitch64,STT_FUNC;

   //===============================================
