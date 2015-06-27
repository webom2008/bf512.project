#include <sys/anomaly_macros_rtl.h>
#include "adi_ebiu_module.h"
.import "adi_ebiu_module.h";



/* If the processor has a DDR memory interface */
#if defined(__ADSP_MOAB__)
/* then define this directive */
#define __ADI_EBIU_USE_DDR_MEMORY__
#endif


#if !defined(__ADI_EBIU_USE_DDR_MEMORY__)
// MMR locations required
#ifndef EBIU_SDGCTL
#define EBIU_SDGCTL 0xFFC00A10
#endif
#ifndef EBIU_SDSTAT
#define EBIU_SDSTAT 0xFFC00A1C
#endif
#endif

// Assembler directives removed; SDRAM base is 0 for all processors thus far

#define ADI_EBIU_SDRAM_START_ADDR 0x00000000


.extern _adi_ebiu_settings;
	.type _adi_ebiu_settings, STT_OBJECT;

#define START_FUNC(Name) \
.global Name; \
.type Name,STT_FUNC; \
.align 4; \
Name:

#define END_FUNC(Name) \
.##Name##.end:

//#define __ADI_EBIU_DEBUG__
.section L1_code;


#if !defined(__ADI_EBIU_USE_DDR_MEMORY__)


//////////////////////////////////////////////////////////////////
// void section("L1_code")
// adi_ebiu_ApplyConfig(
//			u32 isdgctl,
//			u16 isdbctl,
//			u16 isdrrc,
//			int ipowerup)
//

// arguments
#define sdgctl R0
#define sdbctl R1
#define sdrrc  R2
#define powerup R3
#define current_CL R4

#define pSDGCTL_mmr P0
#define pSDBCTL_mmr P1
#define pSDRRC_mmr  P2
#define pSDSTAT_mmr P3

//other 
#define enable_mask R7

START_FUNC(_adi_ebiu_ApplyConfig)

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	//// start prologue
	link  20;
	[--SP] = (R7:4,P5:3);
	//// end proloque	
	/////////////////////////////////////////////////////////////////////////////////////////////////////
	
	// dereference *SDBCTL argument to get true value of register
	P0 = R1;
	
	// load up P-regs with MMR locations
	P1.H = HI(EBIU_SDBCTL); P1.L = LO(EBIU_SDBCTL);
	P2.H = HI(EBIU_SDRRC);  P2.L = LO(EBIU_SDRRC);
	P3.H = HI(EBIU_SDSTAT); P3.L = LO(EBIU_SDSTAT);
	
	// retrieve required EBIU_SDBCTL contents
#if defined(__ADSP_TETON__)
	// 32Bit register for Teton
	R1 = [P0];
#else
	// otherwise 16 bit
	R1 = W[P0](Z);
#endif
	// load up P0 with remaining MMR locations (to optimize cycle count)
	P0.H = HI(EBIU_SDGCTL); P0.L = LO(EBIU_SDGCTL);
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////
	// if SDRS set, clear it with SDRAM access
	// This sets the SDRAM in a known state
	
#if defined(__ADSP_TETON__)

	R5 = [pSDBCTL_mmr];

	// Form mask of Bank enable bits,
	R7.H = HI((1 + (1<<8) + (1<<16) + (1<<24))); R7.L = LO((1 + (1<<8) + (1<<16) + (1<<24)));
	// determine active set
	R7 = R5 & R7; 
	
	// test if any are enabled
	CC = R7 == 0; 
	if CC jump .get_cur_cl;

#else	

	R5 = W[pSDBCTL_mmr](Z);
	CC = BITTST(R5,0);
	if !CC jump .get_cur_cl;
	
#endif

	P4.H = HI(ADI_EBIU_SDRAM_START_ADDR); P4.L = LO(ADI_EBIU_SDRAM_START_ADDR);	
	
#if WA_05000245
	nop;
#endif	

	R5 = [P4];
	//jump .get_cur_cl;
	/////////////////////////////////////////////////////////////////////////////////////////////////////
	
	


.set_self_refresh:	
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////
	// Set self-refresh
#if  WA_05000198  
	nop;
#endif	
	R5 = [pSDGCTL_mmr];
	BITCLR(R5,23);  // clear PSSE bit
	BITSET(R5,24);	// set SRFS bit
	[pSDGCTL_mmr] = R5;
.waiton_self_refresh:
	R5 = W[pSDSTAT_mmr](Z);
	CC = BITTST(R5,1);
	if !CC jump .waiton_self_refresh;
	
.get_cur_cl:		
	// load current CAS Latency (CL) value
#if defined(__ADSP_TETON__) 
 // suppress assembler warning for 05-00-0428 as speculative MMR access are safe
.MESSAGE/SUPPRESS 5517 FOR 2 LINES;  
#endif	
	R4 = [pSDGCTL_mmr];
	R5.L = 0x0202;  //CL
	nop;
	current_CL = EXTRACT(R4,R5.L)(Z);
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////
	//// start critical region
	// It is essential that nothing else happens, since if the code and/or stack is in SDRAM 
	// then we must not go there until this section is complete.
	CLI R6;

	
	/////////////////////////////////////////////////////////////////////////////////////////////////////
	// Disable SDRAM
#if defined(__ADSP_TETON__)
	R5 = [pSDBCTL_mmr];

	// Form mask of Bank enable bits,
	R3.H = HI((1 + (1<<8) + (1<<16) + (1<<24))); 
	R3.L = LO((1 + (1<<8) + (1<<16) + (1<<24)));
	// determine active set
	R7 = R5 & R3; 
	// and disable active set
	R7 = ~R7;
	R5 = R5 & R7; 
	// restore R7 value
	enable_mask = ~R7;
	powerup = [FP + 20]; //4th argument

#if WA_05000198 
	nop;
#endif	
	[pSDBCTL_mmr] = R5;
#else
	R5 = W[pSDBCTL_mmr](Z);
	BITCLR(R5,0);
	R5 = R5.L;
	powerup = [FP + 20]; //4th argument
#if WA_05000198 
	nop;
#endif	
	W[pSDBCTL_mmr] = R5;
#endif
	// wait for SDRAM idle before continuing
.waiton_disable:
	R5 = W[pSDSTAT_mmr](Z);
	CC = ! BITTST(R5,0);
	if CC jump .waiton_disable;

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	// Load MMR's
	
	// EBIU_SDRRC
	CC = sdrrc == 0;
	if CC jump .set_sdbctl;
	R5 = R2.L;
	W[pSDRRC_mmr] = R5;
	
	// EBIU_SDBCTL
.set_sdbctl:
	CC = sdbctl == 0;
	if CC jump .set_sdgctl;
#if defined(__ADSP_TETON__)
	// enable mask needs to be set to that required by SDBCTL argument
	enable_mask = R3 & R1;
	[pSDBCTL_mmr] = R1;
#else
	R5 = R1.L;
	W[pSDBCTL_mmr] = R5;
#endif

	// EBIU_SDGCTL
.set_sdgctl:
	R5 = 0;	
	
	[pSDGCTL_mmr] = R5;
	[pSDGCTL_mmr] = sdgctl;
	csync;
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////
	// Test for power up flag or for CL change - and perform Powerup sequence if either is set
	// Otherwise, simply enable SDRAM by setting EBE in EBIU_SDBCTL
	
	// load jump reg 
	P4.H = .config_complete; P4.L = .config_complete; 
	
	// Perform Powerup sequence if necessary
	CC = powerup == 1;
	if CC jump .powerup_sequence_start;

	// load new CL value
	R5.L = 0x0202;  //CL
	nop;
	R5 = EXTRACT(R0,R5.L)(Z);
	// and test
	CC = current_CL == R5;	
	if CC jump .enable_sdram;

.powerup_sequence_start:	
	/////////////////////////////////////////////////////////////////////////////////////////////////////
	// Powerup sequence - if requested by flag or CL changed
	P4.H = .powerup_sequence_end; P4.L = .powerup_sequence_end; 
#if defined(__ADSP_TETON__) 
 // suppress assembler warning for 05-00-0428 as speculative MMR access are safe
.MESSAGE/SUPPRESS 5517 FOR 2 LINES;  
#endif	
	R5 = [pSDGCTL_mmr];
	BITSET(R5,23);
	[pSDGCTL_mmr] = R5;
	csync;
	
.enable_sdram:
	/////////////////////////////////////////////////////////////////////////////////////////////////////
	// Enable the SDRAM again
#if defined(__ADSP_TETON__)
	R5 = [pSDBCTL_mmr];
	R5 = R5 | enable_mask; 
	[pSDBCTL_mmr] = R5;
	cc = r5==0;
	if !cc jump loop_top;	
loop_top:
#else
	R5 = W[pSDBCTL_mmr](Z);
	BITSET(R5,0);
	R5 = R5.L;
	W[pSDBCTL_mmr] = R5;
#endif
	/////////////////////////////////////////////////////////////////////////////////////////////////////

	jump (P4);
	
.powerup_sequence_end:
	/////////////////////////////////////////////////////////////////////////////////////////////////////
	// complete power up sequence by reading from SDRAM and
	// waiting for completion of sequence
	P4.H = HI(ADI_EBIU_SDRAM_START_ADDR); P4.L = LO(ADI_EBIU_SDRAM_START_ADDR);	
	R5 = [P4];
.waiton_powerup:
#if WA_05000198
	nop;
#endif	
	R5 = W[pSDSTAT_mmr](Z);
	CC = BITTST(R5,2);
	if CC jump .waiton_powerup;
	STI R6;	
	jump .adi_ebiu_ApplyConfig_epilogue;	
	/////////////////////////////////////////////////////////////////////////////////////////////////////
.config_complete:
	
	STI R6;
	//// end critical region	
	/////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////

	// Read external memory to clear self-refresh
 	P4.H = HI(ADI_EBIU_SDRAM_START_ADDR); 
 	P4.L = LO(ADI_EBIU_SDRAM_START_ADDR);	
 	R5 = [P4];
	[P4] = R5;
	FLUSH [P4];

#if  WA_05000198 
	nop;
#endif	

.adi_ebiu_ApplyConfig_epilogue:	
	/////////////////////////////////////////////////////////////////////////////////////////////////////
	//// start epilogue
#if WA_05000245 
	nop; nop;
#endif	
	(R7:4,P5:3) = [SP++];
	P0 = [FP+ 4];
	unlink;
	JUMP (P0);
	//// end epiloque	
	/////////////////////////////////////////////////////////////////////////////////////////////////////

END_FUNC(_adi_ebiu_ApplyConfig)
		

.section program;

//.section L1_code;
//////////////////////////////////////////////////////////////////
// void adi_ebiu_SelfRefreshEnable(void)
//

// local variables
#define ebiu_sdgctl R3
#define ebiu_sdstat R2
#define bSRFS	 24

START_FUNC(_adi_ebiu_SelfRefreshEnable)
	

	P1.H = HI(EBIU_SDBCTL); 
	P1.L = LO(EBIU_SDBCTL);	
	
	/* check bit 0 of SDBCTL to see if SDRAM is enabled */
	R1 = [P1];	
	CC = BITTST(R1,0);
	IF !CC JUMP .adi_ebiu_ExitSelfRefreshEnable;		

	CLI R0;

	// Read external memory to clear previous self-refresh
	// or SDRS (power up on next access)
	P1.H = HI(ADI_EBIU_SDRAM_START_ADDR); 
	P1.L = LO(ADI_EBIU_SDRAM_START_ADDR);	
	R1 = [P1];
	[P1] = R1;
	FLUSH [P1];

	P2.H = HI(EBIU_SDGCTL); 
	P2.L = LO(EBIU_SDGCTL);
	ebiu_sdgctl = [ P2 ];
	BITSET(ebiu_sdgctl,bSRFS);
	//CLI R0;
	[ P2 ] = ebiu_sdgctl;
	//STI R0;	
	P2.H = HI(EBIU_SDSTAT);	
	P2.L = LO(EBIU_SDSTAT);
	
.adi_ebiu_SelfRefreshEnable.wait:
	ebiu_sdstat = W[ P2 ](Z);
	CC = BITTST(ebiu_sdstat,1);
	if !CC jump .adi_ebiu_SelfRefreshEnable.wait;
	STI R0;	
.adi_ebiu_ExitSelfRefreshEnable:
	RTS;
	
END_FUNC(_adi_ebiu_SelfRefreshEnable)




#if defined(__ADSP_TETON__) && !defined(ADI_SSL_SINGLE_CORE)

/////////////////////////////////////////////////////////////////////////////////////////////////////
// ADI_INT_HANDLER adi_pwr_CoreBSuppInt0Handler(void *)
//
// Acknowledge Supplemental Interrupt
//

.extern _adi_pwr_acquire_lock;
	.type _adi_pwr_acquire_lock, STT_OBJECT;
	
.section L1_code;

START_FUNC(_adi_ebiu_SyncCoreA)

/////////////////////////////////////////////////////////////////////////////////////////////////////
	//// start prolog
	// Note: SP could be in SDRAM , but as long as the critical region
	//       below does not use SP or FP then we should be OK
	link  20;
	[--SP] = (R7:4,P5:3);
	//// end proloq
	/////////////////////////////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	//// start Critical Region
	CLI R4;
	
	// Acknowledge Supplemental Interrupt 0
	P0.H = HI(SICB_SYSCR); P0.L = LO(SICB_SYSCR);
	R3 = 0x0800(Z);
	W[P0] = R3;

	// pend on lock (if pointer set)
	CC = R0==0;
	IF CC jump _adi_ebiu_SyncCoreA.exit;
	R5 = R0;
	CALL.X _adi_pwr_acquire_lock;
	
	// and release it again (we have no reason to hold on to it)
	P1 = R5;	
	R1 = 0;
	NOP; NOP; NOP;
	B[P1] = R1;
	
_adi_ebiu_SyncCoreA.exit:
	STI R4;
	//// end Critical Region
	/////////////////////////////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	//// start epilog
	(R7:4,P5:3) = [SP++];
	P0 = [FP+ 4];
	unlink;
	JUMP (P0);
	//// end epiloq	
	/////////////////////////////////////////////////////////////////////////////////////////////////////

END_FUNC(_adi_ebiu_SyncCoreA)

#endif /* teton */

#else

/* This section is for EBIU's that support DDR memory */

/* define which bit to set in the EBIU_RSTCTL register to 
 request a self refresh */
 
#define bSELF_REFRESH_REQUEST 3
#define bSELF_REFRESH_ACK 4



//#define __ADI_EBIU_DEBUG__
.section L1_code;



//////////////////////////////////////////////////////////////////
// void section("L1_code")
// adi_ebiu_ApplyDDRConfig(
//			u32 ddrctl0,
//			u32 ddrctl1,
//			u32 ddrctl2 )
//



START_FUNC(_adi_ebiu_ApplyDDRConfig)

/* start prologue */
	link  20;
	[--SP] = (R7:4,P5:3);
/* end proloque	 */
	

	
	/* delay for SDRAM access to complete (if stack is in DDR) */
	
	nop; nop; nop; nop;


	/* start critical region */
	CLI R6;
		
	
	/* load up P-regs with MMR locations */
	P0.H = HI(EBIU_DDRCTL0); 
	P0.L = LO(EBIU_DDRCTL0);
	P1.H = HI(EBIU_DDRCTL1); 
	P1.L = LO(EBIU_DDRCTL1);
	P2.H = HI(EBIU_DDRCTL2); 
	P2.L = LO(EBIU_DDRCTL2);
	
	/* get the value in DDRCTL2 reg, just get burst length, which is hard-coded */
	R3 = [P2];
	R4 = 0x03;
	R3 = R3 & R4;
    
	/* meanwhile place the other values in the appropriate locations */
	[P0] = R0;
	CSYNC;
	
    /* new DDRCTL2 value should exclude the burst length, so it doesn't get ORd with the hard-coded value */   
	R4 = ~0x03;

	/* meanwhile place the other values in the appropriate locations */	
	[P1] = R1;
	CSYNC;
	
	/* AND out the burst length field from the new CTL2 value,
	  OR together the new value (less burst length) with the previous contents of burst length */
	R2 = R2 & R4;
	
	R2 = R2 | R3;
	
	[P2] = R2;
	CSYNC;
	   
	/*  end critical region	 */	
	
	/* optimal number of loops not yet determined */
	R2.L = 6000;
	R2.H = 0x0;
		
.adi_ebiu_wait_before_accessing_stack:	
	R2 += -1;
	CC = R2 == 0;
	if !CC jump .adi_ebiu_wait_before_accessing_stack;



	
	STI R6;
	
	/* start epilogue */
	
	(R7:4,P5:3) = [SP++];
	P0 = [FP+ 4];
	UNLINK;
	
	/* end epiloque	*/	
	JUMP (P0);

END_FUNC(_adi_ebiu_ApplyDDRConfig)


 

.section program;

//.section L1_code;
//////////////////////////////////////////////////////////////////
// void adi_ebiu_DDRSelfRefreshEnable(void)
//
 
// local variables
#define ebiu_rstctl R2
START_FUNC(_adi_ebiu_DDRSelfRefreshEnable)

	P0.H = HI(EBIU_RSTCTL); 
	P0.L = LO(EBIU_RSTCTL);	

	/* Get contents of DDR control register */
	R1 = W[ P0 ](Z);
	
	/* make sure the DDR controller is active */
	CC = BITTST(R1, 0 );
	IF !CC JUMP .adi_ebiu_ExitDDRSelfRefreshEnable;

	CLI R0;
	P2.H = HI(EBIU_RSTCTL); 
	P2.L = LO(EBIU_RSTCTL);
	ebiu_rstctl = W[ P2 ](Z);
	BITSET(ebiu_rstctl, bSELF_REFRESH_REQUEST);
 
	W[ P2 ] = ebiu_rstctl;
	csync;
			
.adi_ebiu_DDRSelfRefreshEnable.wait:
	ebiu_rstctl = W[ P2 ](Z);
	CC = BITTST(ebiu_rstctl, bSELF_REFRESH_ACK);
	if !CC jump .adi_ebiu_DDRSelfRefreshEnable.wait;
	STI R0;	
.adi_ebiu_ExitDDRSelfRefreshEnable:
	RTS;
	
END_FUNC(_adi_ebiu_DDRSelfRefreshEnable)



// local variables
#define ebiu_rstctl R2
START_FUNC(_adi_ebiu_DDRSelfRefreshDisable)
 
	CLI R0;

	P2.H = HI(EBIU_RSTCTL); 
	P2.L = LO(EBIU_RSTCTL);

	ebiu_rstctl = W[ P2 ](Z);

	BITCLR(ebiu_rstctl, bSELF_REFRESH_REQUEST);
	W[ P2 ] = ebiu_rstctl;

	CSYNC;	

	STI R0;	
	RTS;
	
END_FUNC(_adi_ebiu_DDRSelfRefreshDisable)



#endif /* DDR */

//////////////////////////////////////////////////////////////////
// Function to write the asynchronous memory registers

.section L1_code;


//////////////////////////////////////////////////////////////////
// void section("L1_code")
// adi_ebiu_ApplyAMConfig(
//	u16 amgctl,
//	u32 ambctl0,
//	u32 ambctl1 );
	


START_FUNC(_adi_ebiu_ApplyAMConfig)


	/* start critical region */
	CLI R3;
		

	
	P1.H = HI(EBIU_AMBCTL0); 
	P1.L = LO(EBIU_AMBCTL0);
	
	P2.H = HI(EBIU_AMBCTL1); 
	P2.L = LO(EBIU_AMBCTL1);
	
	/* load up P-regs with MMR locations */
	P0.H = HI(EBIU_AMGCTL); 
	P0.L = LO(EBIU_AMGCTL);
    

	/* write the value of AMBCTL1 - a 32 bit register */   
	[P1] = R1;
	CSYNC;
	/* write the value of AMBCTL1 - a 32 bit register */       	
	[P2] = R2;
	CSYNC;
	
	/* write the value of AMGCTL last - a 16 bit register */
	W[P0] = R0.L;
	CSYNC;
	/*  end critical region	 */	
	STI R3;	
	RTS;

END_FUNC(_adi_ebiu_ApplyAMConfig)
 