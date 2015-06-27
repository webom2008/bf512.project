/******************************************************************************
  Copyright(c) 2000-2004 Analog Devices Inc. IPDC BANGALORE, India. 
  All rights reserved
 ******************************************************************************
  File name   :  atan.asm

  Module name : Fractinal arc tangent.

  Label name  : _atan_fr16
 
  Description :   This program finds the arc tangent of x.

  atan Approximation: y = atan(x)

  Registers used :
  
  R0 - INPUT1 value, 
  R1,R2,R3,R7,P0,P1
 
  CYCLE COUNT    : 12            N == -1
                 : 14            N ==  1
				 : 33            other N
  'N' - INPUT VALUE IN FRACTIONAL FORMAT

  CODE SIZE      : 100 BYTES
  
  DATE           : 26-02-01
**************************************************************/
.file_attr libGroup      = math.h;
.file_attr libGroup      = math_bf.h;
.file_attr libFunc       = __atan_fr16;
.file_attr libFunc       = atan_fr16;
.file_attr libName = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr FuncName      = __atan_fr16;

.section data1;
 .align 2;
 .atancoef:
 .byte2 = 0x7FE3,0x0192,0xCD26,0x1361,0x0758,0xFB34 ;
 
.section  program;
.global __atan_fr16;
.align 2;

__atan_fr16:
            
				R1 = -32768(X);				    // INITIALISE R1 = -1
				CC = R0 == R1;				    // CHECK FOR -1
				IF CC JUMP RET_NEG_MAX;			// RETURN 0X9B78
				R1 = 0X7FFF;				    // INITIALISE R1 = 0X7FFF
				CC = R0 == R1;				    // CHECK FOR 1
				IF CC JUMP RET_POS_MAX;   	    // RETURN 0X6488            
				[--SP] = R7;                    // PUSH R7 REG TO STACK 
				P0.L = .atancoef;  		        // POINTER TO ARRAY OF COEFFICIENTS
				P0.H = .atancoef;
			   	P1 = 3;					   	    // INITIALISE LOOP COUNTER VALUE
		   		R1 = ABS R0;			        // R1 = ABS R0
				R7 = R1;					    // STORE R1
			    			   		
				A0 = 0 || R3 = W[P0++] (Z);			// INITIALISE A0 = 0// GET FIRST COEFICIENT
			    LSETUP(ATANST,ATANEND)LC0 = P1; // SETUP LOOP COUNTER FOR VALUE P1
ATANST:         R7.H = R7.L * R1.L;
				R2 = (A0 += R1.L * R3.L) || R3 = W[P0++] (Z);                 
				R1.L = R7.L * R7.H;
ATANEND:		R2 = (A0 += R7.H * R3.L) || R3 = W[P0++] (Z);                 
				R2.L = R2 (RND);                // ROUND R2
			    R2 = R2.L (X);	               	// GET SIGN EXTEND VALUE
				CC = R0 < 0;		            // CHECK R0 < 0
				R0 = -R2;						// NEGATE THE RESULT
				IF !CC R0 = R2;                 // IF NO SIGN, NON - NEGATED RESULT
				R7 = [SP++];             		// POP R7
				RTS;

RET_NEG_MAX:    R0 = -25735;					// RETURN 0X9B78	
			    R0 = R0.L(X);                   // SIGN EXTEND	
				RTS;

RET_POS_MAX:    R0 = 0X6488;					// RETURN 0X6488		
                RTS; 
.__atan_fr16.end:
              

		
