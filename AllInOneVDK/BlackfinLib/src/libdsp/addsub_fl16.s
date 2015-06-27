/*
** Float16 addition and subtraction.  Takes two non-IEEE-754 floating point 
** numbers (16.16) and either performs an addition or a subtraction
**
** Copyright (C) 2008-2009 Analog Devices, Inc. All Rights Reserved.
*/

#include<sys/anomaly_macros_rtl.h>

.file_attr libGroup      = float16.h;
.file_attr libFunc       = _add_fl16;
.file_attr libFunc       = _sub_fl16;
.file_attr libFunc       = add_fl16;
.file_attr libFunc       = sub_fl16;
.file_attr libName = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr FuncName      = _add_fl16;
.file_attr FuncName      = _sub_fl16;

.section program;
.align 2;

_sub_fl16:
   R2 = 0;
   R1.H = R2.H - R1.H (S);

_add_fl16:
   P1 = R7;

   /* Push both arguments onto stack, it makes it easier to extract and 
      sign extend the high halves */

   [SP + 8] = R0;
   [SP + 4] = R1;

   R0 = W[SP + 8]  (X);     // load exponent y (ye)
   R1 = W[SP + 4]  (X);     // load exponent x (xe)
   R3 = W[SP + 10] (X);     // load mantissa y (ym)
   R7 = W[SP + 6]  (X);     // load mantissa x (xm)

   /* Give zero values the smallest possible exponent 
       if (xm == 0) xe = -32768;
       if (ym == 0) ye = -32768;
   */
   R2 = -32768 (X);

   CC = R7 == 0;
   IF CC R1 = R2;

   CC = R3 == 0;
   IF CC R0 = R2;

   /* Perform the addition 
     ie = max(xe, ye);
     im = (int)(xm >> (ie - xe)) + (ym >> (ie - ye));
   */
   R2 = MAX(R1, R0);
   R2 = R2.L (X);

   R0 = R2 - R0;         // (ie - ye)
   R1 = R2 - R1;         // (ie - xe)
   R7 >>>= R1;
   R3 >>>= R0;
   R1 = R3 + R7;

   /* Normalize the result 
     n = norm_fr1x32(im);
     fl.l = im << n;                     // Keep only the upper 16 bits 
     fl.s.e = sub_fr1x16(ie, (n - 16));  // Exponent is 16-bit saturated
   */
   R7 = P1;              // restore R7
   R0.L = SIGNBITS R1;
   R1 = ASHIFT R1 BY R0.L;
   [SP + 8] = R1;

   R0 += -16;
   R1.L = R2.L - R0.L (S);
   W[SP + 8] = R1;
   R0 = W[SP + 10] (Z);  // load mantissa of result, if it's zero, return zero

   /* Convert our tiny-exponent zero to a normal one */
   CC = R0 == 0;
   if !CC R0 = R1;
   RTS;

._add_fl16.end:
._sub_fl16.end:
.global _add_fl16;
.type _add_fl16, STT_FUNC;
.global _sub_fl16;
.type _sub_fl16, STT_FUNC;
