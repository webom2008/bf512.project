/* $Revision: 5506 $
 * $Date: 2011-07-26 10:45:48 -0400 (Tue, 26 Jul 2011) $
******************************************************************************
Copyright (c), 2009-2010 - Analog Devices Inc. All Rights Reserved.
This software is proprietary &amp; confidential to Analog Devices, Inc.
and its licensors.
******************************************************************************

Title:        Entry point for ISR.

Description:  This files contains entry and exit points for ISR functions.
              Actual interrupt service routine  which is registered by the user 
              will be called once processor registers are stored in a specific
              order. These registers will be restored once execution of the 
              user specified function is completed. Each IVG level has have its 
              own entry point while one common exit point. Once the code specific
               to IVG level is executed code flow will be branched to the label 
              "osal_common_exit_point_for_nested_isr" (if it ISRs are nested)
             
                
*****************************************************************************/

/*=============  I N C L U D E S   =============*/

/* TODO: Path to this file has to be set in the project options.
         The assembler does not pass the standard include paths 
         when it calls the compiler.
*/
#include "../include/osal_common.h"
.import  "../include/osal_common.h";



/*=============  D E F I N E S  =============*/

.file_attr libGroup=adi_osal.h;
.file_attr libName=libosal;
.file_attr prefersMem=internal;
.file_attr prefersMemNum="30";


/* This definition is used to mark an IVG table entry as invalid: that IVG level 
   cannot be used by the user */
#define IVG_ENTRY_INVALID           0X0000

/* define locations of the code and data segments */
#define adi_osal_fast_prio0_code    program
/* some blackfin processors don't have l1_data_b */
#if defined(__ADSPBF50x__) || defined (__ADSPBF531__) || defined(__ADSPBF59x__)
#define adi_osal_fast_prio0_r       L1_data_a
#else
#define adi_osal_fast_prio0_r       L1_data_b
#endif

/* This macro defines a function for specified "IVG" level.
   It calls the processor context saving function and loads 
   the table,which hold the registered functions, to "P0". Program execution 
   branches to the common exit path.
*/
#define ADI_OSAL_NESTED_ISR_FUNCTION(IVG,CODE_SECTION) \
    .section  CODE_SECTION; \
    .align 8; \
    .global _Evt##IVG##NestedEntry; \
    _Evt##IVG##NestedEntry: \
    [--sp] = reti; \
    [--sp] = rets; \
    call.x save_cpu_context;\
    p0.h = hi(__adi_osal_gIsrInfoTable + sizeof(ADI_OSAL_ISR_INFO)*IVG + offsetof(ADI_OSAL_ISR_INFO,pIsrFunction));\
    p0.l = lo(__adi_osal_gIsrInfoTable + sizeof(ADI_OSAL_ISR_INFO)*IVG + offsetof(ADI_OSAL_ISR_INFO,pIsrFunction));\
    jump osal_common_exit_point_for_nested_isr;\
._Evt##IVG##NestedEntry.end:


#define ADI_OSAL_NON_NESTED_ISR_FUNCTION(IVG,CODE_SECTION) \
      .section  CODE_SECTION; \
      .align 8; \
      .global _Evt##IVG##NonNestedEntry; \
      _Evt##IVG##NonNestedEntry: \
      [--sp]=rets;\
       call.x save_cpu_context;\
      p0.h = hi(__adi_osal_gIsrInfoTable + sizeof(ADI_OSAL_ISR_INFO)*IVG + offsetof(ADI_OSAL_ISR_INFO,pIsrFunction));\
      p0.l = lo(__adi_osal_gIsrInfoTable + sizeof(ADI_OSAL_ISR_INFO)*IVG + offsetof(ADI_OSAL_ISR_INFO,pIsrFunction));\
      jump osal_common_exit_point_for_non_nested_isr;\
._Evt##IVG##NonNestedEntry.end:


 
/*=============  E X T E R N A L S  =============*/

.extern   __adi_osal_gIsrInfoTable;

/*=============  D A T A  =============*/
/*
    Holds the address of ISR entry functions. This table is used by
    adi_osal_ISRInstall() API. Mapping of ISR entry function is simplified
    by using this table.
*/
.section adi_osal_fast_prio0_r;
.align 4;

/* following table holds the address of the entry functions for different IVG levels. 
   IVG_ENTRY_INVALID is filled in the places which does not have entry function. These IVG levels are
    are:
      1.) Emulation
      2.) Reset  
      3.) Exception
      4.) Reserved -IPEND[4].
*/
.global __adi_osal_apNonNestedEvtAddressbuffer;
.byte4  __adi_osal_apNonNestedEvtAddressbuffer[ADI_MAX_NUM_IVG_LEVELS] = IVG_ENTRY_INVALID,      /* emulator */
                                                                         IVG_ENTRY_INVALID,      /* reset */
                                                                         _Evt2NonNestedEntry,    /* NMI */
                                                                         IVG_ENTRY_INVALID,      /* Exceptions */
                                                                         IVG_ENTRY_INVALID,      /* Reserved */
                                                                         _Evt5NonNestedEntry,
                                                                         _Evt6NonNestedEntry,
                                                                         _Evt7NonNestedEntry,
                                                                         _Evt8NonNestedEntry,
                                                                         _Evt9NonNestedEntry,
                                                                         _Evt10NonNestedEntry,
                                                                         _Evt11NonNestedEntry,
                                                                         _Evt12NonNestedEntry,
                                                                         _Evt13NonNestedEntry,
                                                                         _Evt14NonNestedEntry,
                                                                         _Evt15NonNestedEntry;
                                                                         
.global __adi_osal_apNestedEvtAddressbuffer;
.byte4  __adi_osal_apNestedEvtAddressbuffer[ADI_MAX_NUM_IVG_LEVELS] = IVG_ENTRY_INVALID,        /* emulator */
                                                                      IVG_ENTRY_INVALID,        /* reset */
                                                                      _Evt2NestedEntry,         /* NMI */
                                                                      IVG_ENTRY_INVALID,        /* Exceptions */
                                                                      IVG_ENTRY_INVALID,        /* Reserved */
                                                                      _Evt5NestedEntry,
                                                                      _Evt6NestedEntry,
                                                                      _Evt7NestedEntry,
                                                                      _Evt8NestedEntry,
                                                                      _Evt9NestedEntry,
                                                                      _Evt10NestedEntry,
                                                                      _Evt11NestedEntry,
                                                                      _Evt12NestedEntry,
                                                                      _Evt13NestedEntry,
                                                                      _Evt14NestedEntry,
                                                                      _Evt15NestedEntry;


/*=============  C O D E  =============*/

/*****************************************************************************
macro "ADI_OSAL_NESTED_ISR_FUNCTION" expands into a function.

No entry functions for following interrupt:

EMU  - IPEND[0] - Emulator exception       - IVG0
RST  - IPEND[1] - Reset exception          - IVG1
EVX  - IPEND[3] - Exception                - IVG3
EMU  - IPEND[4] - Global Interrupt Status.  

FIXME: There  should  be a flexibility to place the correspondig to each EVT level 
       in different section. This can be done by replacing "adi_osal_fast_prio0_code"
       with the desired section name in the macro "ADI_OSAL_NESTED_ISR_FUNCTION"

*****************************************************************************/

ADI_OSAL_NESTED_ISR_FUNCTION(2,adi_osal_fast_prio0_code)

ADI_OSAL_NESTED_ISR_FUNCTION(5,adi_osal_fast_prio0_code)

ADI_OSAL_NESTED_ISR_FUNCTION(6,adi_osal_fast_prio0_code)

ADI_OSAL_NESTED_ISR_FUNCTION(7,adi_osal_fast_prio0_code)

ADI_OSAL_NESTED_ISR_FUNCTION(8,adi_osal_fast_prio0_code)

ADI_OSAL_NESTED_ISR_FUNCTION(9,adi_osal_fast_prio0_code)

ADI_OSAL_NESTED_ISR_FUNCTION(10,adi_osal_fast_prio0_code)

ADI_OSAL_NESTED_ISR_FUNCTION(11,adi_osal_fast_prio0_code)

ADI_OSAL_NESTED_ISR_FUNCTION(12,adi_osal_fast_prio0_code)

ADI_OSAL_NESTED_ISR_FUNCTION(13,adi_osal_fast_prio0_code)

ADI_OSAL_NESTED_ISR_FUNCTION(14,adi_osal_fast_prio0_code)

ADI_OSAL_NESTED_ISR_FUNCTION(15,adi_osal_fast_prio0_code)

/* This is common exit point  for all registered nested interrupts.  */
osal_common_exit_point_for_nested_isr:
    /* Read the registered function address*/
    p1 = [p0];
         
    /* call the registered function */
    call (p1);
    
    /* Restore the processor context */ 
    
    call.x restore_cpu_context;

    rets=[sp++];
    
    reti=[sp++];    
    
    rti;
    
/*****************************************************************************
No entry functions for following interrups:
EMU  - IPEND[0] - Emulater exception       - IVG0
RST  - IPEND[1] - Reset exception          - IVG1
EVX  - IPEND[3] - Exception                - IVG3
EMU  - IPEND[4] - Global Interrupt Status. 
 
FIXME: There  should  be a flexibility to place the correspondig to each EVT level 
       in different section. This can be done by replacing "adi_osal_fast_prio0_code"
       with the desired section name in the macro "ADI_OSAL_NON_NESTED_ISR_FUNCTION"
*****************************************************************************/


    ADI_OSAL_NON_NESTED_ISR_FUNCTION(2,adi_osal_fast_prio0_code)


    ADI_OSAL_NON_NESTED_ISR_FUNCTION(5,adi_osal_fast_prio0_code)


    ADI_OSAL_NON_NESTED_ISR_FUNCTION(6,adi_osal_fast_prio0_code)


    ADI_OSAL_NON_NESTED_ISR_FUNCTION(7,adi_osal_fast_prio0_code)


    ADI_OSAL_NON_NESTED_ISR_FUNCTION(8,adi_osal_fast_prio0_code)


    ADI_OSAL_NON_NESTED_ISR_FUNCTION(9,adi_osal_fast_prio0_code)


    ADI_OSAL_NON_NESTED_ISR_FUNCTION(10,adi_osal_fast_prio0_code)


    ADI_OSAL_NON_NESTED_ISR_FUNCTION(11,adi_osal_fast_prio0_code)


    ADI_OSAL_NON_NESTED_ISR_FUNCTION(12,adi_osal_fast_prio0_code)


    ADI_OSAL_NON_NESTED_ISR_FUNCTION(13,adi_osal_fast_prio0_code)


    ADI_OSAL_NON_NESTED_ISR_FUNCTION(14,adi_osal_fast_prio0_code)


    ADI_OSAL_NON_NESTED_ISR_FUNCTION(15,adi_osal_fast_prio0_code)
    

/* This is common exit point  for all registered non nested interrupts.  */
     
osal_common_exit_point_for_non_nested_isr:   
    /* Read the registered function address*/
    p1 = [p0];
    
    /* call the registered function */
    call(p1); 
    
    /* restore the processor context*/     
    
    call.x restore_cpu_context;
    
    rets=[sp++];
    
    rti; 
    
/*****************************************************************************
  Function: save_cpu_context
     This function stores the processor context. This is called at the entry point 
     of an ISR.
  Parameters:
      none
  Returns:   
      none
*****************************************************************************/
save_cpu_context:
    [ -- sp ] = ( r7:0 , p5:0 ) ;    
    [ -- sp ] = astat;                 
    [ -- sp ] = fp ;                 
    [ -- sp ] = i0 ;                 
    [ -- sp ] = i1 ;                 
    [ -- sp ] = i2 ;                 
    [ -- sp ] = i3 ;                 
    [ -- sp ] = b0 ;                 
    [ -- sp ] = b1 ;                 
    [ -- sp ] = b2 ;                 
    [ -- sp ] = b3 ;                 
    [ -- sp ] = l0 ;                 
    [ -- sp ] = l1 ;                 
    [ -- sp ] = l2 ;                 
    [ -- sp ] = l3 ;                 
    [ -- sp ] = m0 ;                 
    [ -- sp ] = m1 ;                 
    [ -- sp ] = m2 ;                 
    [ -- sp ] = m3 ;                 
    [ -- sp ] = a0.x ;                 
    [ -- sp ] = a0.w ;                 
    [ -- sp ] = a1.x ;                 
    [ -- sp ] = a1.w ;                 
    [ -- sp ] = lc0 ;                 
    [ -- sp ] = lc1 ;                 
    [ -- sp ] = lt0 ;                 
    [ -- sp ] = lt1 ;                 
    [ -- sp ] = lb0 ;                 
    [ -- sp ] = lb1 ;                 
    r3 = 0 ;                         
    lc1 = r3 ;                         
    lc0 = r3 ;                         
    l0 = 0 ( x ) ;                    
    l1 = 0 ( x ) ;                    
    l2 = 0 ( x ) ;                    
    l3 = 0 ( x ) ;                    
    rts;
save_cpu_context.end:    

/*****************************************************************************
  Function: restore_cpu_context
     This function restores the processor context. This is called at the end of
     of an ISR.
  Parameters:
      none
  Returns:   
      none
*****************************************************************************/

restore_cpu_context:   
    lb1 = [ sp ++ ] ;                
    lb0 = [ sp ++ ] ;                
    lt1 = [ sp ++ ] ;                
    lt0 = [ sp ++ ] ;                
    lc1 = [ sp ++ ] ;                
    lc0 = [ sp ++ ] ;                
    a1.w = [ sp ++ ] ;                
    a1.x = [ sp ++ ] ;                
    a0.w = [ sp ++ ] ;                
    a0.x = [ sp ++ ] ;                
    m3 = [ sp ++ ] ;                
    m2 = [ sp ++ ] ;                
    m1 = [ sp ++ ] ;                
    M0 = [ SP ++ ] ;                
    l3 = [ sp ++ ] ;                
    l2 = [ sp ++ ] ;                
    l1 = [ sp ++ ] ;                
    l0 = [ sp ++ ] ;                
    b3 = [ sp ++ ] ;                
    b2 = [ sp ++ ] ;                
    b1 = [ sp ++ ] ;                
    b0 = [ sp ++ ] ;                
    i3 = [ sp ++ ] ;                
    i2 = [ sp ++ ] ;                
    i1 = [ sp ++ ] ;                
    i0 = [ sp ++ ] ;                
    fp = [ sp ++ ] ;                
    astat = [ sp ++ ] ;                
   ( r7:0 , p5:0 ) = [ sp ++ ] ;    
    rts;
restore_cpu_context.end:
    
/*
**
** EOF: 
**
*/

