/*****************************************************************************
 *
 * cplb_is_misaligned_for_size.asm : $Revision: 1.4.14.1 $
 *
 * Copyright (C) 2009 Analog Devices, Inc. All Rights Reserved.
 *
 *****************************************************************************/

#if defined(__DOCUMENTATION__)

   Function: cplb_address_is_misaligned_for_cplb_size

   Description:

      CPLB initialisation has detected than a cplb address is misaligned
      for the size of the block it is covering. Jump to _adi_fatal_error
      to report non-recoverable error.

   Error conditions:

      None.

   Implementation notes:

      Jump to wrapper call_fatal_error to set up duplicate parameters.

 *****************************************************************************
#endif

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.FILE_ATTR libName=libevent;
.FILE_ATTR FuncName=cplb_address_is_misaligned_for_cplb_size;
.FILE_ATTR libFunc=cplb_address_is_misaligned_for_cplb_size;
.FILE_ATTR prefersMem=internal;
.FILE_ATTR prefersMemNum="30";
#endif

#include <sys/fatal_error_code.h>
.EXTERN __call_fatal_error;

.SECTION program;
.ALIGN 2

cplb_address_is_misaligned_for_cplb_size:
       R1 = _AFE_S_CPLBAddressIsMisalignedForCPLBSize;
       JUMP.X __call_fatal_error;
.cplb_address_is_misaligned_for_cplb_size.end:

.GLOBAL cplb_address_is_misaligned_for_cplb_size;
.TYPE cplb_address_is_misaligned_for_cplb_size, STT_FUNC;

