/************************************************************************
**
** L_mls.asm : $Revision: 1.6 $
**
** (c) Copyright 2001-2004 Analog Devices, Inc.  All rights reserved.
**
*************************************************************************/
.file_attr libGroup=libetsi.h;
.file_attr libName=libetsi;
.file_attr libFunc=L_mls;
.file_attr FuncName=_L_mls;
.file_attr libFunc=_L_mls;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";

#if 0
    C Source:

    fract32   L_mls( fract32 Lv, fract16 v )
    {
       fract32   Temp  ;

       Temp = Lv & (fract32) 0x0000ffff ;
       Temp = Temp * (fract32) v ;
       Temp = L_shr( Temp, (fract16) 15 ) ;
       Temp = L_mac( Temp, v, extract_h(Lv) ) ;

       return Temp ;
    }

#endif

	.section program;

.epctext:

	.align 2;
_L_mls:
	R2 = R0.L (Z);
	R2 *= R1 ;
	R2=R2 >>>  15 (S);
	A0 = R2;
	A0 +=R0.H*R1.L (W32);
#if __SET_ETSI_FLAGS
	CC = AV0;
	IF !CC JUMP no_overflow (bp);
	I0.L = _Overflow;
	I0.H = _Overflow;
	R1 = 1;
	[I0] = R1;
no_overflow:
#endif
	R0 = A0;	
	RTS;

._L_mls.end:
	.global _L_mls;
	.type _L_mls,STT_FUNC;
#if __SET_ETSI_FLAGS
	.extern _Overflow;
	.type _Overflow,STT_OBJECT;
#endif
.epctext.end:

