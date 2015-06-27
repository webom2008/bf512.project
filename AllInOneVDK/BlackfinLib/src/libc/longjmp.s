/* Copyright (C) 2000-2007 Analog Devices Inc., All Rights Reserved,
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
*/
//
// this implementation of longjmp() is taken from Marc Hoffman's ADE system
//
.file_attr libName=libc;
.file_attr libFunc=_longjmp;
.file_attr FuncName=_longjmp;
.file_attr libFunc=longjmp;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";
	.section program;
	.align 2;

_longjmp:
	LINK 0x8 ;
	[ -- SP ] = ( R7:7 , P5:5 ) ;
	[ FP + -4 ] = R0 ;
	[ FP + -8 ] = R1 ;
	P0 = R0 ;
	R0 = [ P0 ++ ] ;
	[ -- SP ] = R0 ;
	P1 = [ P0 ++ ] ;
	P2 = [ P0 ++ ] ;
	P3 = [ P0 ++ ] ;
	P4 = [ P0 ++ ] ;
	P5 = [ P0 ++ ] ;
	FP = [ P0 ++ ] ;
	R0 = [ SP ++ ] ;
	SP = [ P0 ++ ] ;
	[ -- SP ] = R0 ;
	[ -- SP ] = R1 ;
	R0 = [ P0 ++ ] ;
	R1 = [ P0 ++ ] ;
	R2 = [ P0 ++ ] ;
	R3 = [ P0 ++ ] ;
	R4 = [ P0 ++ ] ;
	R5 = [ P0 ++ ] ;
	R6 = [ P0 ++ ] ;
	R7 = [ P0 ++ ] ;
	R0 = [ P0 ++ ] ;
	ASTAT = R0 ;
	// LC0 and LC1 not saved and restored as unnecessary 
	R0 = [ P0 ++ ] ;
	A0.w = R0 ;
	R0 = [ P0 ++ ] ;
	A0.x = R0.L ;
	R0 = [ P0 ++ ] ;
	A1.w = R0 ;
	R0 = [ P0 ++ ] ;
	A1.x = R0.L ;
	R0 = [ P0 ++ ] ;
	I0 = R0 ;
	R0 = [ P0 ++ ] ;
	I1 = R0 ;
	R0 = [ P0 ++ ] ;
	I2 = R0 ;
	R0 = [ P0 ++ ] ;
	I3 = R0 ;
	R0 = [ P0 ++ ] ;
	M0 = R0 ;
	R0 = [ P0 ++ ] ;
	M1 = R0 ;
	R0 = [ P0 ++ ] ;
	M2 = R0 ;
	R0 = [ P0 ++ ] ;
	M3 = R0 ;              // restore user register M3
	R0 = [ P0 ++ ] ;
	L0 = R0 ;
	R0 = [ P0 ++ ] ;
	L1 = R0 ;
	R0 = [ P0 ++ ] ;
	L2 = R0 ;
	R0 = [ P0 ++ ] ;
	L3 = R0 ;
	R0 = [ P0 ++ ] ;
	B0 = R0 ;
	R0 = [ P0 ++ ] ;
	B1 = R0 ;
	R0 = [ P0 ++ ] ;
	B2 = R0 ;
	R0 = [ P0 ++ ] ;
	B3 = R0 ;
	R0 = [ P0 ++ ] ;
	RETS = R0 ;
	R0 = [ SP ++ ] ;
	P0 = [ SP ++ ] ;
	CC = R0 == 0 ;
	IF CC JUMP .L1 ;
	RTS ;
.L1:
	R0 = 1 ;
	RTS ;
._longjmp.end:

	.global _longjmp;
	.type _longjmp,STT_FUNC;
