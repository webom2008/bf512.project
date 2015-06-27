/* Copyright (C) 2000-2007 Analog Devices Inc., All Rights Reserved,
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
*/
//
// this implementation of setjmp() is taken from Marc Hoffman's ADE system
//
.file_attr libName=libc;
.file_attr FuncName=__Setjmp;
.file_attr libFunc=__Setjmp;
.file_attr libFunc=_Setjmp;
.file_attr libFunc=setjmp;
.file_attr libFunc=_setjmp;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";
	.section program;
	.align 2;
__Setjmp:
	[ -- SP ] = R1 ;
	R1 = SP ;
	[ -- SP ] = FP ;
	LINK 0x4 ;
	[ -- SP ] = ( R7:7 , P5:5 ) ;
	[ FP + -4 ] = R0 ;
	[ -- SP ] = P0 ;
	P0 = R0 ;
	R0 = [ SP ++ ] ;
	[ P0 ++ ] = R0 ;
	[ P0 ++ ] = P1 ;
	[ P0 ++ ] = P2 ;
	[ P0 ++ ] = P3 ;
	[ P0 ++ ] = P4 ;
	[ P0 ++ ] = P5 ;
// FP
	P1 = R1;
	p1 += -4 ;
	P2 = [ P1 + 0 ] ;
	[ P0 ++ ] = P2 ;
// SP
	P1 = R1 ;
	P1 += 4 ;
	[ P0 ++ ] = P1 ;
// R0
	P1 = [ FP +  -4 ] ;
	[ P0 ++ ] = P1 ;
// R1
	P1 = R1 ;
	P2 = [ P1 + 0 ] ;
	[ P0 ++ ] = P2 ;
	[ P0 ++ ] = R2 ;
	[ P0 ++ ] = R3 ;
	[ P0 ++ ] = R4 ;
	[ P0 ++ ] = R5 ;
	[ P0 ++ ] = R6 ;
	[ P0 ++ ] = R7 ;
	R0 = ASTAT ;
	[ P0 ++ ] = R0 ;
	// LC0 and LC1 not saved and restored as unnecessary
	R0 = A0 ;
	[ P0 ++ ] = R0 ;
	R0.L = A0.x ;
	[ P0 ++ ] = R0 ;
	R0 = A1.w ;
	[ P0 ++ ] = R0 ;
	R0.L = A1.x ;
	[ P0 ++ ] = R0 ;
	R0 = I0 ;
	[ P0 ++ ] = R0 ;
	R0 = I1 ;
	[ P0 ++ ] = R0 ;
	R0 = I2 ;
	[ P0 ++ ] = R0 ;
	R0 = I3 ;
	[ P0 ++ ] = R0 ;
	R0 = M0 ;
	[ P0 ++ ] = R0 ;
	R0 = M1 ;
	[ P0 ++ ] = R0 ;
	R0 = M2 ;
	[ P0 ++ ] = R0 ;
	R0 = M3 ;              // save user register M3 
	[ P0 ++ ] = R0 ;
	R0 = L0 ;
	[ P0 ++ ] = R0 ;
	R0 = L1 ;
	[ P0 ++ ] = R0 ;
	R0 = L2 ;
	[ P0 ++ ] = R0 ;
	R0 = L3 ;
	[ P0 ++ ] = R0 ;
	R0 = B0 ;
	[ P0 ++ ] = R0 ;
	R0 = B1 ;
	[ P0 ++ ] = R0 ;
	R0 = B2 ;
	[ P0 ++ ] = R0 ;
	R0 = B3 ;
	[ P0 ++ ] = R0 ;
	R0 = RETS ;
	[ P0 ++ ] = R0 ;
	( R7:7 , P5:5 ) = [ SP ++ ] ;
	UNLINK ;
	SP += 8 ;
	R0 = 0 ;
	RTS ;
.__Setjmp.end:

	.global __Setjmp;
	.type __Setjmp,STT_FUNC;
