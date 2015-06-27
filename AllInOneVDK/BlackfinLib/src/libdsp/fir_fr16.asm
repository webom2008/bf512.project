/******************************************************************************
Copyright(c) 2000-2008 Analog Devices Inc.
*******************************************************************************
File Name      : fir_fr16.asm
Function Name  : __fir
Module Name    : FILTER Library
Description    : This function performs FIR filter operation on given input.
Operands       : R0 - Address of input vector,
                 R1 - Address of output vector,
                 R2 - Number of input elements
                 Filter structure is on stack.

Prototype      : void fir( const fract16 x[],
                           fract16 y[],
                           int n,
                           fir_state_fr16 *s ); 

                 Structure of fir_state_fr16:
                 {
                      fract16 *h,   // filter coefficients 
                      fract16 *d,   // start of delay line 
                      fract16 *p,   // read/write pointer 
                      int k,        // no. of coefficients 
                      int l         // interpolation/decimation index 
                 } fir_state_fr16;

Registers used : R0, R1, R2, R3, R5, R6, R7   

                 I0 -> Address of delay line (for updating the delay line)
                 I1 -> Address of delay line (for fetching the delay line data)
                 I2 -> Address of filter coeff. h0, h1 , ... , hn-1
                 I3 -> Address of output array y[]
                 P0 -> Address of input array x[]
                 P1, P2
                 P5 -> Address of structure s

                 Delay Line (Num Coeff = 5)
                 Initial Set-up:   | x-4 | x-5 | x-1 | x-2 | x-3 |
                                   I1 -> before loop samples pointing at x-2
                                   I0 -> before loop samples pointing at x-5 

Code size      : 354 bytes

Cycle Counts   : Even Taps, Even no. inputs:  64 + Ni/2*(3 + Nc)
                                              for Ni=256, L=8:   1472

                 Odd Taps, Even no. inputs:   70 + Ni/2*(3 + Nc)
                                              for Ni=256, L=9:   1606

                 Even Taps, Odd no. inputs:   64 + (Ni-1)/2*(3 + Nc) + Nc
                                              for Ni=257, L=8: 1480

                 Odd Taps, Odd no. inputs:    73 + (Ni-1)/2*(3 + Nc) + Nc
                                              for Ni=257, L=9: 1618

******************************************************************************/

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.file_attr libGroup      = filter.h;
.file_attr libFunc       = __fir_fr16;
.file_attr libFunc       = fir_fr16;
.file_attr libName = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr FuncName      = __fir_fr16;
#endif

#include <sys/anomaly_macros_rtl.h>

#if defined(__ADSPLPBLACKFIN__) && defined(__WORKAROUND_AVOID_DAG1)
#define __WORKAROUND_BF532_ANOMALY38__
#endif

#if !defined(__ADSPLPBLACKFIN__)
/* Suppress Assembler Warning ea5504 "Memory store instruction use may trigger
** hardware anomaly 05-00-0165". The run-time libraries are not required to
** workaround this anomaly.
*/

.MESSAGE/SUPPRESS 5504;
#endif

.section program;
.global    __fir_fr16;

.align 2;
__fir_fr16:

       [--SP]=(R7:5);             // PUSH R7-R5,P5 ON STACK
       [--SP]=P5;

       P5=[SP+28];                // ADDRESS OF FILTER STRUCTURE
       P0=R0;                     // ADDRESS OF INPUT ARRAY    
       I3=R1;                     // ADDRESS OF OUTPUT ARRAY
       CC=R2<=0;                  // CHECK IF NUMBER OF INPUT ELEMENTS<=0

       P1=[P5++];                 // POINTER TO FILTER COEFFICIENTS
       P2=[P5++];                 // POINTER TO DELAY LINE
       R6=[P5++];                 // ADDRESS TO READ/WRITE POINTER
       R1=[P5--];                 // NUMBER OF COEFFICIENTS

       // EXIT IF NUMBER OF INPUT ELEMENTS <=0
       IF CC JUMP _fir_fr16_RET_END;

       // EXIT IF NUMBER OF COEFFS <=0
       CC=R1<=0;                  // CHECK IF NUMBER OF COEFF. <=0
       IF CC JUMP _fir_fr16_RET_END;        

       R5=R1;                     // STORE NUMBER OF FILTER COEFF. IN R5
       R1=R1+R1;                  // DOUBLE R1 TO INCREMENT AS HALF WORD
       I2=P1;                     // INITIALISE I2 TO FILTER COEFF. ARRAY
       B2=P1;                     // MAKE FILTER COEFFS. AS CIRCULAR BUFFER 
       L2=R1;
       I1=R6;                     // INDEX TO READ/WRITE POINTER
       B1=P2;                     // INITIALISE B1 AND L1
       L1=R1;                     // FOR  DELAY LINE CIRCULAR BUFFER
       I0=R6;                     // INDEX TO READ/WRITE POINTER
       B0=P2;                     // INITIALISE B0 AND L0 
       L0=R1;                     // FOR  DELAY LINE CIRCULAR BUFFER

       P1=R2;                     // SET OUTER LOOP COUNTER
       P2=R5;                     // SET INNER LOOP COUNTER
       L3=0;

       R6=PACK(R2.H,R2.L) || I1+=2;
       CC=R6==1;

       // BRANCH IF SINGLE SAMPLE 
       IF CC JUMP _fir_fr16_SING_SAMP;

       // BRANCH IF NUMBER OF FILTER COEFF. <= 3  
       CC=R5<=3;
       IF CC JUMP _fir_fr16_SING_SAMP;  

       CC=BITTST(R5,0);           // CHECK IF THE NUMBER OF FILTER TAPS ARE ODD
       P2+=-2;                    // Nc-2

       // BRANCH IF NUMBER OF COEFFS ODD
       IF CC JUMP _fir_fr16_FIR_ODD;

#if WA_05000428                             || \
    defined(__WORKAROUND_CSYNC)             || \
    defined(__WORKAROUND_SPECULATIVE_LOADS)
       NOP;
       NOP;
#endif


       /****************************************************
                   EVEN NUMBER OF INPUT SAMPLES
                   EVEN NUMBER OF COEFFICIENTS
       ****************************************************/

#if defined(__WORKAROUND_BF532_ANOMALY38__)

       I1+=2;
       I1+=2 || R2=[I2++];
       R3.L = W[I0++] || R0=[P0++];

       // Loop 1 to Ni/2
       LSETUP(_fir_fr16_E_FIR_START,_fir_fr16_E_FIR_END) LC0=P1>>1;
_fir_fr16_E_FIR_START:
               A1=R0.H*R2.L,A0=R0.L*R2.L ||I1-=2 || W[I0--]=R0.L;
               R7=PACK(R0.H,R7.L) || R0.H=W[I1++];

               //Loop 1 to Nc-1/2
               LSETUP(_fir_fr16_E_MAC_ST,_fir_fr16_E_MAC_END) LC1=P2>>1;
_fir_fr16_E_MAC_ST:    R2.L=W[I2++];
                       A1+=R0.L*R2.H,A0+=R0.H*R2.H || R0.L=W[I1++];
                       R2.H=W[I2++];
_fir_fr16_E_MAC_END:   A1+=R0.H*R2.L,A0+=R0.L*R2.L || R0.H=W[I1++];

               R3.H=(A1+=R0.L*R2.H),R3.L=(A0+=R0.H*R2.H) || 
                                         R0=[P0++] || W[I0--]=R7.H;

_fir_fr16_E_FIR_END:
               R2=[I2++] || [I3++]=R3;

#else  /* End of BF532 Anomaly#38 Safe Code */

       I1+=2;  
       I0+=2 || R0.L=W[I1++];
       R0=[P0++] || R2=[I2++];

       // Loop 1 to Ni/2
       LSETUP(_fir_fr16_E_FIR_START,_fir_fr16_E_FIR_END) LC0=P1>>1;
_fir_fr16_E_FIR_START:
               A1=R0.H*R2.L,A0=R0.L*R2.L ||I1-=2 || W[I0--]=R0.L;
               R7=PACK(R0.H,R7.L) || R0.H=W[I1++] || NOP;

               //Loop 1 to Nc-1/2
               LSETUP(_fir_fr16_E_MAC_ST,_fir_fr16_E_MAC_END)LC1=P2>>1;
_fir_fr16_E_MAC_ST:    A1+=R0.L*R2.H,A0+=R0.H*R2.H || 
                                          R2.L=W[I2++] || R0.L=W[I1++];
_fir_fr16_E_MAC_END:   A1+=R0.H*R2.L,A0+=R0.L*R2.L || 
                                          R2.H=W[I2++] || R0.H=W[I1++];

               R3.H=(A1+=R0.L*R2.H),R3.L=(A0+=R0.H*R2.H) || 
                                          R0=[P0++] || W[I0--]=R7.H;
_fir_fr16_E_FIR_END:
               R2=[I2++] || [I3++]=R3;

#endif /* End of Alternative to BF532 Anomaly#38 Safe Code */

       R0.L=W[I0--] || I2-=4;

#if defined(__WORKAROUND_INFINITE_STALL_202) && \
   !defined(__WORKAROUND_BF532_ANOMALY38__)
/* After the dual dag load/store, already performed one load. Thus
** only one prefetch is required to avoid the anomaly sequence.
*/
       PREFETCH[SP];
#endif

       R0=I0;
       R2.L=W[I1--];
       P0+=-4;

       JUMP _fir_fr16_DATA;


       /****************************************************
                   EVEN NUMBER OF INPUT SAMPLES
                   ODD NUMBER OF COEFFICIENTS
       ****************************************************/

_fir_fr16_FIR_ODD:

       B3=I3;
       R2>>=1;
       R7.H=W[I1++] || R5=[P0];
       R2=R2 << 2 || R6.H=W[I1++];
       L3=R2;

#if defined(__WORKAROUND_BF532_ANOMALY38__)

       R2=[I2++] || I3-=4;
       R3=[I3];

       //Loop 1 to Ni/2
       LSETUP(_fir_fr16_O_FIR_START,_fir_fr16_O_FIR_END) LC0=P1>>1;
_fir_fr16_O_FIR_START:
               R7.H=W[I0] || W[I0++]=R5.H;
               R0=PACK(R6.H,R5.L) || R1=[P0++];
               A1=R5.H*R2.L,A0=R5.L*R2.L || R5=[P0] || W[I0--]=R5.L;

               //Loop 2 to Nc/2-1
               LSETUP(_fir_fr16_O_MAC_ST,_fir_fr16_O_MAC_END) LC1=P2>>1;
_fir_fr16_O_MAC_ST:    A1+=R0.L*R2.H,A0+=R0.H*R2.H || R2=[I2++];
                       R0.L=W[I1++];
_fir_fr16_O_MAC_END:   A1+=R0.H*R2.L,A0+=R0.L*R2.L || R0.H=W[I1++];

               A1+=R0.L*R2.H,A0+=R0.H*R2.H || R2.H=W[I2++] || I0-=4;

               //Done only for Odd number of coeffs
               R3.L=(A0+=R7.H*R2.H) || R6.H=W[I1++] ||  [I3++]=R3 ;

_fir_fr16_O_FIR_END:
               R3.H=(A1+=R0.H*R2.H) || R2=[I2++];

#if defined(__WORKAROUND_INFINITE_STALL_202)
/* After the dual dag load/store, already performed one load. Thus
** only one prefetch is required to avoid the anomaly sequence.
*/
       PREFETCH[SP];
#endif

#else  /* End of BF532 Anomaly#38 Safe Code */

       R7.H=W[I0] || I3-=4;
       R3=[I3] || R2=[I2++];

       //Loop 1 to Ni/2                
       LSETUP(_fir_fr16_O_FIR_START,_fir_fr16_O_FIR_END) LC0=P1>>1;
_fir_fr16_O_FIR_START:

#if defined(__WORKAROUND_INFINITE_STALL_202)
/* To avoid the anomaly, break-up the first dual dag load/store */
               W[I0++]=R5.H ;
               R0=PACK(R6.H,R5.L) || R1=[P0++];
#else
               R0=PACK(R6.H,R5.L) || R1=[P0++] || W[I0++]=R5.H ;
#endif
               A1=R5.H*R2.L,A0=R5.L*R2.L || R5=[P0] || W[I0--]=R5.L;

               //Loop 2 to Nc/2-1
               LSETUP(_fir_fr16_O_MAC_ST,_fir_fr16_O_MAC_END) LC1=P2>>1;
_fir_fr16_O_MAC_ST:    A1+=R0.L*R2.H,A0+=R0.H*R2.H || 
                                              R2=[I2++] || R0.L=W[I1++];
_fir_fr16_O_MAC_END:   A1+=R0.H*R2.L,A0+=R0.L*R2.L || 
                                              R0.H=W[I1++];

               A1+=R0.L*R2.H,A0+=R0.H*R2.H || R2.H=W[I2++] || I0-=4;

               //Done only for Odd number of coeffs
               R3.L=(A0+=R7.H*R2.H) || R6.H=W[I1++] || [I3++]=R3 ;

_fir_fr16_O_FIR_END:
               R3.H=(A1+=R0.H*R2.H) || R2=[I2++] || R7.H=W[I0]; 
                    
#if defined(__WORKAROUND_INFINITE_STALL_202)
/* After 2 dual dag load/stores, need 2 prefetch loads to avoid the
** anomaly (since the first prefetch is still part of the anomaly sequence).
*/
       PREFETCH[SP];
       PREFETCH[SP];
#endif

#endif /* End of Alternative to BF532 Anomaly#38 Safe Code */

       L3 = 0;
       R0 = I0;
       I2-=4;
       [I3++] = R3 || I1-=2;


_fir_fr16_DATA:
       CC=BITTST(R6,0);

       //If even number of input samples, jump to ret_end
       IF !CC JUMP _fir_fr16_RET_END1;

       P2+=2;
       P1=1; 
       I1-=2; 


       /****************************************************
                   SINGLE INPUT SAMPLE
                   LAST ELEMENT WHERE SAMPLE LENGTH ODD
       ****************************************************/

_fir_fr16_SING_SAMP:

#if WA_05000428                             || \
    defined(__WORKAROUND_CSYNC)             || \
    defined(__WORKAROUND_SPECULATIVE_LOADS)
       NOP;
       NOP;
#endif

#if defined(__WORKAROUND_BF532_ANOMALY38__)

       I1+=2;

       // Loop 1 to Ni
       LSETUP(_fir_fr16_SLOW_START,_fir_fr16_SLOW_END) LC0=P1;
_fir_fr16_SLOW_START:

          A1=A0=0 || R0=W[P0++] (Z);
          R7 = PACK(R0.H,R0.L) || R2.H=W[I0++];

          //Loop 1 to Nc
          LSETUP(_fir_fr16_L_MAC_START,_fir_fr16_L_MAC_END)LC1=P2;
_fir_fr16_L_MAC_START:
             R2.L=W[I2++];
_fir_fr16_L_MAC_END:
             R3.L=(A0+=R0.L*R2.L) || R0.L=W[I1++];

          W[I0--]=R7.L || I1-=2;

_fir_fr16_SLOW_END:
          W[I3++]=R3.L || I0-=2;


#else  /* End of BF532 Anomaly#38 Safe Code */

#if WA_05000428                             || \
    defined(__WORKAROUND_CSYNC)             || \
    defined(__WORKAROUND_SPECULATIVE_LOADS)

       NOP;
#endif

       R2.L=W[I2++] || I1+=2;

       // Loop 1 to Ni
       LSETUP(_fir_fr16_SLOW_START,_fir_fr16_SLOW_END) LC0=P1;
_fir_fr16_SLOW_START:

          A1=A0=0 || R0=W[P0++] (Z);
          R7 = PACK(R0.H,R0.L) || R2.H=W[I0++];

#if defined(__WORKAROUND_INFINITE_STALL_202)
          /* After the dual dag load, already performed one load. Thus
          ** only one prefetch is required to avoid the anomaly sequence.
          */
          PREFETCH[SP];
#endif
 
          //Loop 1 to Nc
          LSETUP(_fir_fr16_L_MAC_ST,_fir_fr16_L_MAC_ST)LC1=P2;
_fir_fr16_L_MAC_ST:     
             R3.L=(A0+=R0.L*R2.L) || R2.L=W[I2++] || R0.L=W[I1++];

          W[I0--]=R7.L || I1-=2;

_fir_fr16_SLOW_END:
          W[I3++]=R3.L || I0-=2;          


#endif /* End of Alternative to BF532 Anomaly#38 Safe Code */

       R0=I0;

_fir_fr16_RET_END1:
       [P5]=R0;

_fir_fr16_RET_END:
       L0=0;
       L1=0;
       L2=0;

       P5=[SP++];
       (R7:5)=[SP++];        //POP R7-R5,P5 FROM STACK
       RTS;

.__fir_fr16.end:
