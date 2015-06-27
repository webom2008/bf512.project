/**************************************************************************
  Copyright(c) 2000-2006 Analog Devices Inc.
***************************************************************************
  File Name      : cfir_fr16.asm
  Include File   : filter.h
  Function Name  : __cfir
  Description    : This function performs Complex FIR filter operation.
  Operands       : R0- Address of input vector,
                   R1- Address of output vector,
                   R2- Number of elements
                   Filter structure is on stack.

  Code size      : 108 bytes
  Cycle Counts   : 27 + Ni*(3 + 2*Nc), for Ni=128, Nc=31:  8347
**************************************************************************/

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.file_attr libGroup      = filter.h;
.file_attr libFunc       = __cfir_fr16;
.file_attr libFunc       = cfir_fr16;
.file_attr libName = libdsp;
.file_attr prefersMem    = internal;
.file_attr prefersMemNum = "30";
.file_attr FuncName      = __cfir_fr16;
#endif

#if defined(__ADSPLPBLACKFIN__) && defined(__WORKAROUND_AVOID_DAG1)
#define __WORKAROUND_BF532_ANOMALY38__
#endif

.section  program;
.global    __cfir_fr16;

.align 2;
__cfir_fr16:
            P0=[SP+12];         // ADDRESS OF FILTER STRUCTURE
            I0=R0;              // ADDRESS OF INPUT ARRAY
            I3=R1;              // ADDRESS OF OUTPUT ARRAY
            CC=R2<=0;           // CHECK IF NUMBER OF INPUT ELEMENTS<=0
            R1=[P0++];          // NUMBER OF FILTER COEFFICIENTS
            P1=[P0++];          // POINTER TO FILTER COEFFICIENTS
            P2=[P0++];          // ADDRESS OF DELAY LINE
            R3=[P0];            // THE READ/WRITE POINTER

            IF CC JUMP RET_END;

            CC=R1<=0;           // CHECK IF NUMBER OF FILTER COEFF <=0
            IF CC JUMP RET_END;

            B1=P2;              // MAKE INPUT A CIRCULAR BUFFER
            P2=R1;              // SET INNER LOOP COUNTER
            I2=P1;              // INITIALIZE I2 TO FILTER COEFF. ARRAY
            B2=P1;              // MAKE FILTER COEFF. A CIRCULAR BUFFER

            P1=R2;              // SET OUTER LOOP COUNTER
            R1=R1 << 2 || R0 = [I0]; 
                                // MULTIPLY R1 BY 4 TO FETCH DATA OF WORD LENGTH
                                // FETCH 1ST SAMPLE FROM INPUT
            L2=R1;
            I1=R3;              // READ/WRITE POINTER
            L1=R1;

            M1=8(X);            // INITIALIZE TO MODIFY INPUT DATA BUFFER

            R2=[I2++];

            LSETUP(FIR_START,FIR_END)LC0=P1;
FIR_START:     A1=A0=0 || [I1--]=R0 || R3=[I0++];
                                // INITIALIZE A0 AND A1,
                                // FETCH FILTER COEFF. AND INPUT DATA
                                // STORE INPUT VALUE IN CIRCULAR BUFFER

#if defined(__WORKAROUND_BF532_ANOMALY38__)

       /* Start of BF532 Anomaly#38 Safe Code */

               LSETUP(MAC_ST,MAC_END)LC1=P2;
MAC_ST:           A1 += R3.H*R2.L, A0 += R3.L*R2.L || R0 = [I0];
                  R1.H= (A1+=R3.L*R2.H), R1.L= (A0-=R3.H*R2.H) || R2 = [I2++];
MAC_END:          R3 = [I1--];

#else  /* End of BF532 Anomaly#38 Safe Code */

               LSETUP(MAC_ST,MAC_END)LC1=P2;
MAC_ST:           A1 += R3.H*R2.L, A0 += R3.L*R2.L || R0 = [I0];
MAC_END:          R1.H= (A1+=R3.L*R2.H), R1.L= (A0-=R3.H*R2.H) || 
                                                     R2 = [I2++] || R3=[I1--];
#endif /* End of Alternative to BF532 Anomaly#38 Safe Code */

FIR_END:       MNOP || R3=[I1++M1] ||[I3++]=R1;
                                // STORE THE RESULT AND
                                // DUMMY FETCH TO KEEP POINTER IN PROPER POSITION

#if defined(__WORKAROUND_INFINITE_STALL_202) && \
    !defined(__WORKAROUND_BF532_ANOMALY38__)
/* After 2 dual dag load/stores, need 2 prefetch loads to avoid the
** anomaly (since the first prefetch is still part of the anomaly sequence).
*/
            PREFETCH[SP];
            PREFETCH[SP];
#endif

            R0=I1;              // GET THE READ/WRITE POINTER
            [P0]=R0;            // STORE THE READ WRITE POINTER FOR NEXT OPERATION

            L1=0(X);
            L2=0(X);
RET_END:
            RTS;

.__cfir_fr16.end:
