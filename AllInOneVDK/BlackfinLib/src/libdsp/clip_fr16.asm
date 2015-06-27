/*****************************************************************
	Copyright(c) 2000-2004 Analog Devices Inc. IPDC BANGALORE, India.
	All rights reserved
 *****************************************************************				  

    File name   :   clip_fr16.asm 
	Module name :   Fractional clip
	Label name  :   __clip_fr16
	Description : 	This program clips a 16 bit input number to a preset CLIP value .
     

	Registers used   :   

	R0 - INPUT value, X	(32 bits)
	R1 - CLIP value   	(32 bits)	

	Other registers used:
	R2, R3

	Cycle count		:	12 cycles per sample
	
	Code size		:	22 bytes


 *******************************************************************/	

.file_attr libGroup      = math_bf.h;
.file_attr libGroup      = math.h;
.file_attr libFunc       = __clip_fr16;
.file_attr libFunc       = clip_fr16;
.file_attr libName = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr FuncName      = __clip_fr16;

.section  program;
.align 2;
.global __clip_fr16;
__clip_fr16:

		R2 = R0;				// COPY INPUT 
		R3 = ABS R0;			// TAKE ABSOLUTE VALUE OF THE INPUT
		R1 = ABS R1;			// TAKE ABSOLUTE VALUE OF CLIP VALUE 
		R0 = -R1;				// LET RETURN VALUE BE THE NEGATIVE OF ABS(CLIP)
		CC = R2 < 0;			// CHECK IF INPUT IS NEGATIVE 
		IF !CC R0 = R1;			// IF POSITIVE RETURN VALUE COULD BE ABS(CLIP)
		CC = R3 < R1;			// CHECK IF ABS(CLIP) < ABS(INPUT)
		IF CC R0 = R2;			// IF TRUE RETURN INPUT
		RTS;				
.__clip_fr16.end:
