// **************************************************************************
//
//	Analog Devices Blackfin ETSI Implementation. 
// 	Copyright (C). 2007 Analog Devices Inc., All Rights Reserved.
//
// **************************************************************************
.file_attr libGroup=libetsi.h;
.file_attr libName=libetsi;
.file_attr libFunc=Div_32;
.file_attr FuncName=_Div_32;
.file_attr libFunc=_Div_32;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";

        .section program;
.epctext:
        .align 2;
_Div_32:
	// ---- IN: R0 (L_num), R1.L (denom_hi), R2.L (denom_lo)
	LINK 0x28;
	[FP +8] = R0;
	[FP +12] = R1;
	[FP +16] = R2;
	R0.L = 0x3FFF ;
	// ---- approx = div_s ( 0x3FFF, denom_hi)
	CALL.X _div_s;
	// R0 = approx
	[FP -24 ] = R0;
	R2 = R0;
	R0 = [FP + 12];
	R1 = [FP + 16];
	// ---- L32 = MPY 32 16 denom_hi, denom_lo, approx (R0)
	CALL.X _Mpy_32_16;
	// R0 = L32
	R1 = R0;
	R0.H = 0x7FFF;
	R0.L = 0xFFFF;
	// ---- L32 = L_sub(0x7fffffffL,L32)
	CALL.X _L_sub;
	// RO = L32
	[FP -20] = R0; // L32
	R1 = FP;	// hi
	R1 += -16;
	R2 = FP;	// lo
	R2 += -12;
	// ---- L_Extract(L32,&hi,&lo)
	CALL.X _L_Extract;
	// hi,lo stored. L32 clobbered out of R0.
	
	R0 = [ FP -16 ];
	R1 = [ FP -12 ];
	R2 = [ FP -24 ];
	// ---- L32 = Mpy_32_16(hi,lo,approx)
	CALL.X _Mpy_32_16;
	[FP -20 ] = R0 ; // Store L_32;
	R1 = FP;
	R1 += -16;
	R2 = FP;
	R2 += -12;
	// ---- L_Extract(L_32,&hi,&lo);
	CALL.X _L_Extract;
	R1 = FP;
	R1 += -8;
	R2 = FP;
	R2 += -4;
	R0 = [FP +8];
	// ---- L_Extract(L_num,&n_hi,&n_lo)
	CALL.X _L_Extract;
	R0 = [FP -8];
	R1 = [FP -4];
	R2 = [FP -16];
	R3 = [FP -12];
	[SP + 12 ] = R3;
	// ---- L32 = Mpy_32(n_hi,n_lo,hi,lo)
	CALL.X _Mpy_32;
	// RO = L32
	R1 = 2;
	// ---- L32 = L_shl(L_32,2)
	CALL.X _L_shl;
	UNLINK;
	RTS;
	
._Div_32.end:
        .global _Div_32;
        .type _Div_32,STT_FUNC;
#if __ETSI_DIV_CHECKS
	.extern _abort;
	.type _abort,STT_FUNC;
#endif
#if __SET_ETSI_FLAGS
        .extern _Overflow;
        .type _Overflow,STT_OBJECT;
#endif
	.extern _L_shl;
	.type _L_shl,STT_FUNC;
	.extern _Mpy_32;
	.type _Mpy_32,STT_FUNC;
	.extern _L_Extract;
	.type _L_Extract,STT_FUNC;
	.extern _Mpy_32_16;
	.type _Mpy_32_16,STT_FUNC;
	.extern _L_sub;
	.type _L_sub,STT_FUNC;
	.extern _div_s;
	.type _div_s,STT_FUNC;
.epctext.end:
        .section data1;


