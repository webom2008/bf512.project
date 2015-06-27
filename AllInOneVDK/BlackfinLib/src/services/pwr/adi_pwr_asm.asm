#include <sys/anomaly_macros_rtl.h>
#include "adi_pwr_module.h"
.import "adi_pwr_module.h";


#if !defined(__ADSP_MOY__) && !defined(__ADSP_DELTA__) /* no EBIU */
// MMR locations required
#ifndef EBIU_SDGCTL
#define EBIU_SDGCTL 0xFFC00A10
#endif
#ifndef EBIU_SDSTAT
#define EBIU_SDSTAT 0xFFC00A1C
#endif

#endif

#ifndef PLL_CTL
#define PLL_CTL 0xFFC00000
#endif
#ifndef PLL_DIV
#define PLL_DIV 0xFFC00004
#endif
#ifndef VR_CTL
#define VR_CTL  0xFFC00008
#endif


#define START_FUNC(Name) \
.global Name; \
.type Name,STT_FUNC; \
.align 4; \
Name:

#define END_FUNC(Name) \
.##Name##.end:
    
.extern _adi_pwr_active_config;
    .type _adi_pwr_active_config, STT_OBJECT;
      
#define bSELF_REFRESH_REQUEST 3
#define bSELF_REFRESH_ACK 4

 
/*
 * When changing clock frequencies external memory must first be placed into self-refresh else
 * the contents of external memory will be corrupted.
 * The self refresh routine will need to address external memory to enter/exit
 * self-refresh.
 *
 * This address is also read by the EBIU service to complete power up sequence by reading from SDRAM.
 *
 * The address given below is the default location used by the power and EBIU routines.
 * If your custom target board does not have external memory at this default location you will
 * need to edit this macro and rebuild the system service library.
 *
 * The same macro exists in the file adi_ebiu_asm.asm and will also need to be changed there.
 */
#define ADI_EBIU_SDRAM_START_ADDR 0x00000000


#if defined(__ADSP_TETON__) 
// Use old version for Teton, because it requires IDLE each time the function is called.
// This function is only called on core A

//////////////////////////////////////////////////////////////////
// ADI_PWR_RESULT adi_pwr_program_pll(ADI_PWR_CONFIG *config, int resetPLL)
//                
//
//  Assumes all registers are to change

.section L1_code;

START_FUNC(_adi_pwr_program_pll)

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    //// start prologue
    link  20;
    [--SP] = (R7:4,P5:3);
    //// end proloque    
    /////////////////////////////////////////////////////////////////////////////////////////////////////


    
    // base address of ADI_PWR_CONFIG structure
    
    P4 = R0;
    
    // set flag for pll programming sequence
    CC = R1==0;
    
    // load P-regs with addresses of MMRs
    P0.H = HI(PLL_CTL); 
    P0.L = LO(PLL_CTL);
    
    // Load D-regs with values to apply
    nop;    
    R0 = W[P4 + offsetof(ADI_PWR_CONFIG,pll_ctl)](Z);
    R1 = W[P4 + offsetof(ADI_PWR_CONFIG,pll_div)](Z);
    R2 = W[P4 + offsetof(ADI_PWR_CONFIG,pll_lockcnt)](Z);
// insert VR_CTL code if not Edinburgh core or if __SILICON_REVISION__ not defined ('none'?)or only if not 'any' or '0.1'
#if !defined(__ADSP_EDINBURGH__) || !defined(__SILICON_REVISION__) || (__SILICON_REVISION__!=0xffff && __SILICON_REVISION__!=0x1)
    R3 = W[P4 + offsetof(ADI_PWR_CONFIG,vr_ctl)](Z);
#endif

    CLI R5;
    
    
    // write MMRs
    W[P0]      = R0;  // PLL_CTL
    W[P0 + 4]  = R1;  // PLL_DIV
    W[P0 + 16] = R2;  // PLL_LOCKCNT
#if !defined(__ADSP_EDINBURGH__) || !defined(__SILICON_REVISION__) || (__SILICON_REVISION__!=0xffff && __SILICON_REVISION__!=0x1)
    W[P0 + 8] = R3;   // VR_CTL
#endif

    if CC jump .adi_pwr_program_pll_exit;
    
#if WA_05000244 // defined(__WORKAROUND_SPECULATIVE_SYNCS)
    nop; nop; nop;
#endif
    IDLE;
    
.adi_pwr_program_pll_exit:
    
    STI R5;
    
    // workaround anomaly 05-00-0428
#if  WA_05000428 //defined(__ADSP_TETON__) 
    nop;
#endif            
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    //// start epilogue
    (R7:4,P5:3) = [SP++];
    P0 = [FP+ 4];
    unlink;
    JUMP (P0);
    //// end epiloque    
    /////////////////////////////////////////////////////////////////////////////////////////////////////

END_FUNC(_adi_pwr_program_pll)

#else

// new version, for all processors except Teton, works around some issues with PLL registers being written superfluously.      
//////////////////////////////////////////////////////////////////
// ADI_PWR_RESULT adi_pwr_program_pll(ADI_PWR_CONFIG *config, int resetPLL)
//                
//
//  Assumes all registers are to change

.section L1_code;

START_FUNC(_adi_pwr_program_pll)

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    //// start prologue
    link  20;
    [--SP] = (R7:4,P5:3);
    //// end proloque    
    

        
    /* Get base address of ADI_PWR_CONFIG structure */
    /* Do this before loading P0 to lessen the stall when loading R2 below to 1 cycle */
    P4 = R0;
        

    
    
    /* The ADI_PWR_CONFIG structure cannot be located in external memory */
    /* Accessing it it in external memory while in self-refresh would */
    /* obviously not work. Therefore, the data structure must be */
    /* cached in registers for subsequent access */

    /* The registers available to do so are R7 and P3 */
    /* The caching assignments are as follows */

    /*   PLL_CTL      = R7.H */
    /*   VR_CTL       = R7.L */
    /*   PLL_LOCKCNT  = P3.H */
    /*   PLL_DIV      = P3.L */
    
        
    /* get the value of pll_div */


.MESSAGE/SUPPRESS 1056 FOR 1 LINES;  /* The stall associated with a PREG RAW stall is acceptable */
    R7 = W[P4 + offsetof(ADI_PWR_CONFIG,pll_div)] (Z);
    
    /* get the value of pll_lockcnt  */
    R6 = W[P4 + offsetof(ADI_PWR_CONFIG,pll_lockcnt)] (Z);
    R6 = R6 << 16;
    R7 = R7 | R6;
    P3 = R7;
    
    
    
    /* get the new vr_ctl value from the config structure */
    R7 = W[P4 + offsetof(ADI_PWR_CONFIG, vr_ctl)](Z);

    /* Get the new pll_ctl value from the config structure */    
    R6 = W[P4 + offsetof(ADI_PWR_CONFIG,pll_ctl)](Z);
    R6 = R6 << 16;
    R7 = R7 | R6;
    
    
    
        
#if defined(__ADSP_MOAB__) 
/* Enter Self-Refresh Mode - uses P0, R5 */    

    P0.H = HI(EBIU_RSTCTL); 
    P0.L = LO(EBIU_RSTCTL);    


    /* Get contents of DDR control register */
    R5 = W[ P0 ](Z);
    
    /* make sure the DDR controller is active */
    CC = BITTST(R5, 0 );
    IF !CC JUMP .adi_pwr_AfterSelfRefresh;
        
    /* continue entering self-refresh */
    BITSET(R5, bSELF_REFRESH_REQUEST);
 
    /* disable interrupts for CSYNC */
    CLI R0;
    
    W[ P0 ] = R5;
    CSYNC;
    
    /* reenable interrupts */
    STI R0;
            
.adi_pwr_WaitDDRSelfRefreshEnable:
    R5 = W[ P0 ](Z);
    CC = BITTST(R5, bSELF_REFRESH_ACK);
    if !CC jump .adi_pwr_WaitDDRSelfRefreshEnable;
    
#else
#if !defined(__ADSP_MOY__) && !defined(__ADSP_DELTA__) /* no EBIU */ 

/* Non-DDR Enter Self-Refresh Mode - uses P0, P1, P2, R0, R4, R5 */    
    P0.H = HI(EBIU_SDGCTL); 
    P0.L = LO(EBIU_SDGCTL);
    P1.H = HI(EBIU_SDSTAT); 
    P1.L = LO(EBIU_SDSTAT);
    P2.H = HI(EBIU_SDBCTL); 
    P2.L = LO(EBIU_SDBCTL);        
    
    /* check bit 0 of SDBCTL to see if SDRAM is enabled */
    R5.L = W[P2];    
    CC = BITTST(R5,0);
    IF !CC JUMP .adi_pwr_AfterSelfRefresh;        
    
    CLI R0;    
    

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // if SDRS set, clear it with SDRAM access
    // This sets the SDRAM in a known state    
    
    P0.L = LO(ADI_EBIU_SDRAM_START_ADDR);
    P0.H = HI(ADI_EBIU_SDRAM_START_ADDR);
    R5 = [P0];    
    

    P0.H = HI(EBIU_SDGCTL); 
    P0.L = LO(EBIU_SDGCTL);    
    

/////////////////////////////////////////////////////////////////////////////////////////////////////
    // Put SDRAM into self-refresh - It will come out when SDRAM accessed
    R5 = [P0];
    //BITCLR(R5,23);  // clear PSSE bit
    BITSET(R5,24);    // set SRFS bit
    [P0] = R5;
    CSYNC;    
    
    STI R0;    
    
    P0.L = LO(ADI_EBIU_SDRAM_START_ADDR);
    P0.H = HI(ADI_EBIU_SDRAM_START_ADDR);
    R0 = [P0];
    
        
.adi_pwr_WaitSelfRefreshEnable:
    R4 = W[P1](Z);
    CC = BITTST(R4,1);
    if !CC jump .adi_pwr_WaitSelfRefreshEnable;    

#endif // Moy    
#endif    /* DDR or not */

.adi_pwr_AfterSelfRefresh:    
    
    /* load P-regs with addresses of MMRs */
    P0.H = HI(PLL_CTL); 
    P0.L = LO(PLL_CTL);
    
    /* start out saying that VR_CTL does not need to be written */
    R6 = 0;    

    /* add a NOP to prevent a stall when accessing P4 in the next instruction */
    NOP;
        
    /* get the value of pll_div and write to the MMR */
    R2 = P3;
    

    /* get the value of pll_lockcnt and write to the MMR */
    
    /* write the values */
    R3 = R2.L (Z);
    W[P0 + 4]  = R3;     /* PLL_DIV */
    R3 = R2;
    R3 = R3 >> 16;
    W[P0 + 16] = R3;     /* PLL_LOCKCOUNT */
    
    /* Div and Lockcnt are done - check whether VR_CTL is different */

    
get_vr_ctl:

#if !defined(__ADSP_EDINBURGH__) || !defined(__SILICON_REVISION__) || (__SILICON_REVISION__!=0xffff && __SILICON_REVISION__!=0x1)

    /* get the new vr_ctl value from the config structure */
    R3 = R7.L (Z);

    /* Read the current value of VR_CTL from the MMR */
    R2 = W[P0 + 8](Z);

    
    
/*************************************************************************************
  If using Syscontrol (Moab ) to program the VR we skip all this, 
  first making sure that VLEV is not changed 
*************************************************************************************/
#if defined(__ADSP_MOAB__) /* using Syscontrol to program VR */


    /* use R5 to mask off all except the VLEV bits from old value */
    R5 = 0x00F0;
    R2 = R2 & R5;
    
    /* Now R2 has just the OLD VLEV */
    
    /* use R5 to mask out the VLEV bits from the new value */
    
    R5.L = 0x0FF0F;
    R3 = R3 & R5;
    
    /* Or together the old VLEV with the new value (excluding VLEV ) */
    R3 = R3 | R2;
    
    /* Now R3 has the New Value with the OLD VLEV swapped in */
    
    /* Get the current value of VR_CTL from the MMR again */
    R2 = W[P0 + 8](Z);
    
   
#endif    
    
    
    /* if new value is the same as the current value... */
    CC = R3 == R2;

    /* then don't write new value at all */
    IF CC JUMP get_pll_ctl;    
    
    /* setting R6 means that VR_CTL has changed and must be written-- either now or after pll_ctl is written, 
      either with or without an IDLE, if R6 is 2, we will IDLE, if 1, no IDLE */
      
    R6 = 1;

    
/*************************************************************************************
  If NOT using Syscontrol ( not Moab ) we must determine whether VLEV is
  increasing or decreasing 
*************************************************************************************/

#if !defined(__ADSP_MOAB__)     /* using Syscontrol to program VR */

        
    /*************************************************************************************
     *  SEE WHAT IN VR HAS CHANGED - VLEV???
     *************************************************************************************/

    /* Check VLEV bits use R5 to mask off all except the VLEV bits from both the new value */
    R5 = 0x00F0;
    
    /* and from the old value */
    R2 = R2 & R5;
    R3 = R3 & R5;
    
    /* Compare: Is new VLEV same as old VLEV? */
    CC = R3 == R2;

    
    /* If VLEV hadn't changed, skip on to checking the FREQ and gain bits */
    IF CC JUMP check_freq_gain_bits;        
    
        
    /*************************************************************************************
        VLEV has changed, higher or lower ?
    *************************************************************************************/

    /* We know that the new VLEV is different so increment R6, so we will IDLE when we write VR_CTL */
    R6 += 1;
    
    /* But when do we write VR? We must compare. If new VLEV is higher than old VLEV, we must 
       set VR_CTL before PLL_CTL. But if new VLEV is less than old, we skip from here to the 
       PLL_CTL section BUT WE MUST GET THE NEW VAL BACK IN R3 */
       
       /* if new value is less */
       
    CC = R3 < R2;    
    

    /* Again get the new vr_ctl value from the config structure */
    R3 = R7.L (Z);
    
    IF CC JUMP get_pll_ctl;    
    
    /* if we are here, new VLEV is higher */
    
    

check_freq_gain_bits:
        
    /*************************************************************************************
      If we are here, new VLEV may be higher (write 1st), or the same, so we check 
      freq and gain bits, or VTRIM for kookaburra, because if VLEV didn't change, but 
      these fields did change, we have to execute the IDLE.
     ************************************************************************************/

    /* Again read the current value of VR_CTL from the MMR */
    R2 = W[P0 + 8](Z);
    
    /* if R6 is already 1, we know we have to write VR */
    /* R6 could be 0 here? if VLEV was the same */
    
    
#if defined(__ADSP_KOOKABURRA__)
 
    /* use R5 to mask all except 2 freq bits and 4 VTRIM bits */
 
    R5 = 0x300F;
 
#else 
    
    /* use R5 to mask all except 2 freq bits and 2 gain bits */
    R5 = 0x00F;
     
#endif


    /* from the new value and from the old value */
    R2 = R2 & R5;
    R3 = R3 & R5;
    
    /* Compare: Are these fields in new value same as in old ? */
    CC = R3 == R2;
    
    /* get the new vr_ctl value back again from the config structure for use now or later */
    R3 = R7.L (Z);
            
    IF CC JUMP write_vr_ctl_now;        
    
    /* If they were different, we increment R6 so we know to IDLE when we write the VR_CTL reg. */
    R6 += 1;

write_vr_ctl_now:

    /* lock out interrupts */
    CLI R5;
    
    /*  write new VR_CTL now */
    W[P0 + 8] = R3;   
    CSYNC;    
    
    /* If R6 greater than 1 we must also IDLE */
    CC = R6 < 2;
    IF CC JUMP .adi_pwr_program_pll_skip_vr_idle_1;
    
    R6 = 0;    

    IDLE;  

.adi_pwr_program_pll_skip_vr_idle_1:
    /* restore interrupts */
    STI R5;    

#endif
    

/*************************************************************************************
 ********** End of section which is only executed if NOT using syscontrol *************
**************************************************************************************/
#endif    /* Not using Syscontrol to program VR */

    
get_pll_ctl:
    
    /* Get the new pll_ctl value from the config structure */    
    R2 = R7;
    R2 = R2 >> 16;
    
    /* Read the current value that is in the PLL_CTL MMR */
    R1 = W[P0](Z);

    /* compare new PLL_CTL to old */
    CC = R1 == R2;

    /* don't write new value unless they differ */
    IF CC JUMP check_vr_ctl_flag;

    
    /* lock out interrupts */
    CLI R5;
    
    /* write the new value */
    W[P0] = R2; 

#if WA_05000244 // defined(__WORKAROUND_SPECULATIVE_SYNCS)
    NOP; NOP; NOP;
#endif

    IDLE;  


    /* restore interrupts */
    STI R5;    
    
check_vr_ctl_flag:
    
    /* Check flag to see whether we still need to program vr_ctl */    
    CC = R6 == 0;
    
    /* we don't, go to exit */
    IF CC JUMP .adi_pwr_program_pll_exit;
    
    /* we do... */
#if !defined(__ADSP_EDINBURGH__) || !defined(__SILICON_REVISION__) || (__SILICON_REVISION__!=0xffff && __SILICON_REVISION__!=0x1)

    /* lock out interrupts */
    CLI R5;    
    
    /* write new VR_CTL which is still in R3 
     for Syscontrol processors, we will not change VLEV, R3 will have old VLEV */
    W[P0 + 8] = R3;   
    csync;
    

#if !defined(__ADSP_MOAB__)  
/* Syscontrol processors will never IDLE when changing VR_CTL because they will not change VLEV 
  R6 will be 0 or 1 for the Syscontrol processors. */
        
    /* Check flag to see whether we should IDLE to restart the PLL */    
    CC = R6 < 2;
        
    /* If R6 less than 2, skip the IDLE  */
        
    /* we don't, skip idle */
    IF CC JUMP .adi_pwr_program_pll_skip_vr_idle_2;
        
    
    IDLE;  

    
#endif    /* Syscontrol processors  */

.adi_pwr_program_pll_skip_vr_idle_2:
    /* restore interrupts */
    STI R5;    

        
#endif
        
.adi_pwr_program_pll_exit:
#if defined(__ADSP_MOAB__) 

/* Exit Self-Refresh Mode - uses R0, R5, P0  */    

    CLI R0;

    P0.H = HI(EBIU_RSTCTL); 
    P0.L = LO(EBIU_RSTCTL);

    R5 = W[ P0 ](Z);

    BITCLR(R5, bSELF_REFRESH_REQUEST);
    W[ P0 ] = R5;

    CSYNC;    
    STI R0;    
    
    
.adi_pwr_WaitDDRSelfRefreshDisable:
    R5 = W[ P0 ](Z);
    CC = BITTST(R5, bSELF_REFRESH_ACK);
    if CC jump .adi_pwr_WaitDDRSelfRefreshDisable;
    
#else
#if !defined(__ADSP_MOY__) && !defined(__ADSP_DELTA__)  /* no EBIU */    
    
    /* check bit 0 of SDBCTL to see if SDRAM is enabled */
    R5.L = W[P2];    
    CC = BITTST(R5,0);
    IF !CC JUMP .adi_pwr_AfterExitSelfRefresh;        

    P0.H = HI(EBIU_SDGCTL); 
    P0.L = LO(EBIU_SDGCTL);    
    
    CLI R0;    
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // Take SDRAM out of self-refresh - 
    R5 = [P0];
    BITCLR(R5,24);    // clear SRFS bit
    [P0] = R5;
    CSYNC;    
    
    STI R0;        
    
    
    
/* access external memory to start auto-refresh again */
    P0.L = LO(ADI_EBIU_SDRAM_START_ADDR);
    P0.H = HI(ADI_EBIU_SDRAM_START_ADDR);
    R0 = [P0];
    [P0] = R0;
    FLUSH[P0];

.adi_pwr_WaitSelfRefreshDisable:
    R4 = W[P1](Z);
    CC = BITTST(R4,1);
    if CC jump .adi_pwr_WaitSelfRefreshDisable;
    

.adi_pwr_AfterExitSelfRefresh:
#endif // Moy Delta
#endif // Moab
    
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    //// start epilogue
    (R7:4,P5:3) = [SP++];
    P0 = [FP+ 4];
    unlink;
    JUMP (P0);
    //// end epiloque    
    /////////////////////////////////////////////////////////////////////////////////////////////////////

END_FUNC(_adi_pwr_program_pll)


#endif 
// Use old version for Teton, because it requires IDLE each time the function is called.




/////////////////////////////////////////////////////////////////////////////////////////////////////
//    void adi_pwr_PLLProgrammingSequence( 
//        u16                pll_ctl
//)
// Used to change the power mode wakeup.
// 

.section L1_code;

START_FUNC(_adi_pwr_PLLProgrammingSequence)
    

    // load P-regs with addresses of MMRs
    P0.H = HI(PLL_CTL); 
    P0.L = LO(PLL_CTL);

    // disable interrupts
    CLI R1;
    NOP; NOP;
    
    // write to PLL_CTL MMR
    W[P0] = R0;    
    CSYNC;
    IDLE;  

    
    // restore interrupts
    STI R1;

    
    RTS;
    
END_FUNC(_adi_pwr_PLLProgrammingSequence)

//#define ADI_PWR_MILLIVOLTS(V)    

 

#if defined(__ADSP_TETON__) && !defined(ADI_SSL_SINGLE_CORE)

/////////////////////////////////////////////////////////////////////////////////////////////////////
// void adi_pwr_acquire_lock(
//        testset_t *pLock,     /* R0 */
//      )
//
//
// This function uses R1.
//   _adi_pwr_acquire_lock is only called from _adi_pwr_SyncCoreA
//      which does not store anything in  R1 as this function is called.
//

.section L1_code;

START_FUNC(_adi_pwr_acquire_lock)
        CSYNC;

        /* perform first test, and exit if lock acquired */
        P1 = R0;
        
#if WA_05000248
        /* for applicable silicon to work around anomaly 05-00-0248, 
                   read a L2 address so we can write the same value back */
        P2.L = 0x0000;
        P2.H = 0xFEB0;                    
        R6 = [P2];
#endif
                
        NOP;NOP;NOP;NOP;
        
#if WA_05000412        /* to work around anomaly 05-00-0412 */        
        /* R1 does not need to be saved and can be used for dummy read.
          Perform a dummy read to make sure CPLB is installed */                  
        
        R1 = [P1];    
            
        NOP; NOP;
        SSYNC;
#endif        
        testset(P1);

#if WA_05000248
        /* for applicable silicon to work around anomaly 05-00-0248,
                   write same value back to L2  */
        [P2] = R6;
#endif
        IF CC JUMP  ._adi_pwr_acquire_lock.4 ;
        
        // first test fails to use zero-overhead loop to pend on the lock
._adi_pwr_acquire_lock.l:
        P0 =   -1;
        LSETUP (._adi_pwr_acquire_lock.2 , ._adi_pwr_acquire_lock.3-2) LC0 = P0;
._adi_pwr_acquire_lock.2:

        NOP;NOP;NOP;
        CSYNC;
        
#if WA_05000412        /* to work around anomaly 05-00-0412 */        
        SSYNC;
#endif
        
        testset(P1);


#if WA_05000248        /* for applicable silicon to work around anomaly 05-00-0248,
                   write same value back to L2  */
        [P2] = R6;
#endif

        IF CC JUMP  ._adi_pwr_acquire_lock.4 ;
        NOP;
._adi_pwr_acquire_lock.3:

        JUMP ._adi_pwr_acquire_lock.l;

._adi_pwr_acquire_lock.4:

        RTS;
        
END_FUNC(_adi_pwr_acquire_lock)

/////////////////////////////////////////////////////////////////////////////////////////////////////
// void adi_pwr_SyncCoreA(
//        testset_t *pLockVar,     /* R0 */
//      )
// Function to simply acknowledge SuppInt0, IDLE, and
// wait for the all-clear from Core A
// It is in ASM to guarantee that we are in L1 Memory and that no stack variables
// are used in the critical region where we wait for the PLL and SDRAM adjustments
// (made by Core A) take effect.

.section L1_code;

START_FUNC(_adi_pwr_SyncCoreA)

// How do we know if we are synchronizing with SetFreq or a power mode change?

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
    P0.H = HI(SICB_SYSCR); 
    P0.L = LO(SICB_SYSCR);

    R3 = 0x0800(Z);
    W[P0] = R3;
    csync;
    
    // and IDLE awaiting wake up (if EBIU changed last)
    IDLE;
    
    // pend on lock (if pointer set)
    CC = R0==0;
    IF CC jump _adi_pwr_SyncCoreA.exit;
    R5 = R0;
    CALL.X _adi_pwr_acquire_lock;
    
    // and release it again (we have no reason to hold on to it)
    P1 = R5;    
    R1 = 0;
    NOP;NOP;NOP;
    B[P1] = R1;
    
_adi_pwr_SyncCoreA.exit:
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

END_FUNC(_adi_pwr_SyncCoreA)

#endif
