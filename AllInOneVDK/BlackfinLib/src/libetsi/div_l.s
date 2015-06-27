/************************************************************************
**
** div_l.asm : $Revision: 1.7 $
**
** (c) Copyright 2001-2002 Analog Devices, Inc.  All rights reserved.
**
*************************************************************************/
.file_attr libGroup=libetsi.h;
.file_attr libName=libetsi;
.file_attr libFunc=div_l;
.file_attr FuncName=_div_l;
.file_attr libFunc=_div_l;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";

#if 0

    ETSI comment:
/*__________________________________________________________________________
|                                                                           |
|   Function Name : div_l                                                   |
|                                                                           |
|   Purpose :                                                               |
|                                                                           |
|   Produces a result which is the fractional integer division of L_var1 by|
|   var2; L_var1 and var2 must be positive and var2 << 16 must be greater or|
|   equal to L_var1; the result is positive (leading bit equal to 0) and    |
|   truncated to 16 bits.                                                   |
|   If L_var1 == var2 << 16 then div_l(L_var1,var2) = 32767.                |
|                                                                           |
|   Complexity weight : 20                                                  |
|                                                                           |
|   Inputs :                                                                |
|                                                                           |
|    L_var1                                                                 |
|             32 bit long signed integer (fract32) whose value falls in the  |
|             range : 0x0000 0000 <= var1 <= (var2 << 16)  and var2 != 0.   |
|             L_var1 must be considered as a Q.31 value                     |
|                                                                           |
|    var2                                                                   |
|             16 bit short signed integer (fract16) whose value falls in the |
|             range : var1 <= (var2<< 16) <= 0x7fff0000 and var2 != 0.      |
|             var2 must be considered as a Q.15 value                       |
|                                                                           |
|   Outputs :                                                               |
|                                                                           |
|    none                                                                   |
|                                                                           |
|   Return Value :                                                          |
|                                                                           |
|    var_out                                                                |
|             16 bit short signed integer (fract16) whose value falls in the |
|             range : 0x0000 0000 <= var_out <= 0x0000 7fff.                |
|             It's a Q15 value (point between b15 and b14).                 |
|___________________________________________________________________________|
*/

    C Source:

   fract16 div_l( fract32  L_num, fract16 den ){

       fract16   var_out = (fract16)0;
       fract32   L_den;
       fract16   iteration;

       if ( den == (fract16) 0 ) {
           printf("Division by 0, Fatal error \n");
           exit(0);
       }

       if ( (L_num < (fract32) 0) || (den < (fract16) 0) ) {
           printf("Division Error, Fatal error \n");
           exit(0);
       }

       L_den = L_deposit_h( den ) ;

       if ( L_num >= L_den ){
           return MAX_16 ;
       }
       else {
           L_num = L_shr(L_num, (fract16)1) ;
           L_den = L_shr(L_den, (fract16)1);
           for(iteration=(fract16)0; iteration< (fract16)15;iteration++) {
               var_out = shl( var_out, (fract16)1);
               L_num   = L_shl( L_num, (fract16)1);
               if (L_num >= L_den) {
                   L_num = L_sub(L_num,L_den);
                   var_out = add(var_out, (fract16)1);
               }
           }

           return var_out;
       }
   }

#endif

	.section program;

.epctext:

	.align 2;

		// fract16 div_l( fract32  L_num, fract16 den )
_div_l:

	[--SP]=R7;
	R2 = R0 ;
	// error conditions
	CC =  R1 ==  0;			// den == (fract16) 0
	IF CC JUMP  ._div_error ;
	CC = R2 <  0;				// (L_num < (fract32) 0)
	IF CC JUMP  ._div_error ;
	CC = R1 <  0;				// (den < (fract16) 0) )
	IF CC JUMP  ._div_error ;

	R1 <<=  16;					// L_den = L_deposit_h( den )
	CC = R1 <= R2;				// if ( L_num >= L_den )
	R0 =  32767 (X);			// return MAX_16
	IF CC JUMP  ._finished ;

	P0 =   15;
	R7=R1 >>>  1 (S);			// L_den = L_shr(L_den, (fract16)1);
	R1.L =  0;
	R3.L =  1;
	R0=R2 >>>  1 (S);			// L_num = L_shr(L_num, (fract16)1) ;
#if __SET_ETSI_FLAGS
	// Load I0 with _Overflow outside the loop
	I0.L = _Overflow;
	I0.H = _Overflow;
#endif
	LSETUP (._lstart , ._lend) LC0=P0;
.align 4;

._lstart:						// for(i=0; i< 15;i++)
	R0=R0 <<  1 (S);			// L_num = L_shl( L_num, (fract16)1);
	CC = R7 <= R0;	
	R1.L=R1.L <<  1 (S);		// var_out = shl( var_out, (fract16)1);
	IF !CC JUMP  ._lend ;	// if (L_num >= L_den)
	R0= R0 - R7 (S);			// L_num = L_sub(L_num,L_den);
#if __SET_ETSI_FLAGS
#if !defined(__ADSPLPBLACKFIN__)
	CC = AV0;
#else
	CC = V;
#endif
   IF !CC JUMP no_sub_overflow (BP);
   [I0] = R1;
no_sub_overflow:
#endif
	R1.L = R1.L + R3.L (S);	// var_out = add(var_out, (fract16)1);
#if __SET_ETSI_FLAGS
#if !defined(__ADSPLPBLACKFIN__)
	CC = AV0;
#else
	CC = V;
#endif
   IF !CC JUMP no_add_overflow (BP);
   [I0] = R1;
no_add_overflow:
#endif
._lend:
	NOP;
	// end loop ._lstart;
	R0 = R1.L (X);				// return var_out
._finished:
	R7 = [SP++];
	RTS;

._div_error:
	CALL.X _abort;

._div_l.end:
	.global _div_l;
	.type _div_l,STT_FUNC;

	.extern _abort;
	.type _abort,STT_FUNC;
#if __SET_ETSI_FLAGS
	.extern _Overflow;
	.type _Overflow,STT_OBJECT;
#endif	
.epctext.end:

	.section data1;

