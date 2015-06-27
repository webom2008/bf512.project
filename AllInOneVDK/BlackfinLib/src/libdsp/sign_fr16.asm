/*****************************************************************
	Copyright(c) 2000-2004 Analog Devices Inc. IPDC BANGALORE, India.
	All rights reserved
 *****************************************************************				  

    File name   :   copysign_fr16.asm
	Module name :   Sign copy
	Label name  :   __copysign_fr16
	Description : 	This program attaches the sign of the second
					parameted to firat one
     
	Registers used   :   

	RL0 - First argument		(16 bits)
    RL1 - Second argument   	(16 bits)	

	Other registers used:
	R2

	Cycle count		:	8 cycles per sample

	Code size		:	12 bytes
 *******************************************************************/	

.file_attr libGroup      = math_bf.h;
.file_attr libGroup      = math.h;
.file_attr libFunc       = __copysign_fr16;
.file_attr libFunc       = copysign_fr16;
.file_attr libName = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr FuncName      = __copysign_fr16;

.section  program;
.align 2;
.global __copysign_fr16;
__copysign_fr16:

		R0 = ABS R0;			// TAKE ABS(INPUT) 
		R2 = -R0;
		CC = R1 < 0;			// IF POSITIVE RETURN ABS(INPUT)
   		IF CC R0 = R2;			// IF NEGATIVE, Y = -ABS(CLIP VALUE)  
		RTS;
.__copysign_fr16.end:		




