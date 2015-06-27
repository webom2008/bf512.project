/* Copyright (C) 2003-2006 Analog Devices, Inc. All Rights Reserved. */

/* CPLB table definitions for ADSP-BF532. */

#if defined(__ADSPBF532__)
#include <sys/platform.h>
#include <cplb.h>

.section/DOUBLEANY cplb_data;

.align 4;
// The possible placements of stack in memory must be covered by a locked
// DCPLB to ensure it is always available for use by exception handlers. 
_dcplbs_table:
.byte4=
  // L1 Data A & B, (set write-through bit to avoid 1st write exceptions)
  0xFF800000, (PAGE_SIZE_4MB | CPLB_DNOCACHE | CPLB_LOCK | CPLB_WT),

  0x20200000, (PAGE_SIZE_1MB | CPLB_DNOCACHE),  // Async Memory Bank 2 (Secnd)
  0x20100000, (PAGE_SIZE_1MB | CPLB_DDOCACHE),  // Async Memory Bank 1 (Prim B)
  0x20000000, (PAGE_SIZE_1MB | CPLB_DDOCACHE),  // Async Memory Bank 0 (Prim A)

  // 128 MB SDRAM memory space
  // MEM_SDRAM0_BANK0 (8MB - defined in default LDF) potentially utilised as
  // stack, therefore locked.
  0x00000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE | CPLB_LOCK), 
  0x00400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE | CPLB_LOCK),
  0x00800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x00C00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x01000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x01400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x01800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x01C00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //

  0x02000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x02400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x02800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x02C00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x03000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x03400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x03800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x03C00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //

  0x04000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x04400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x04800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x04C00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x05000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x05400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x05800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x05C00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //

  0x06000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x06400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x06800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x06C00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x07000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x07400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x07800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x07C00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //

  0x20300000, (PAGE_SIZE_1MB | CPLB_DNOCACHE),  // Async Memory Bank 3

  0xffffffff; // end of section - termination
._dcplbs_table.end:


_icplbs_table:
.byte4=
  0xFFA00000, (PAGE_SIZE_1MB | CPLB_I_PAGE_MGMT), // L1 Code

  0x20200000, (PAGE_SIZE_1MB | CPLB_INOCACHE),  // Async Memory Bank 2 (Secnd)
  0x20100000, (PAGE_SIZE_1MB | CPLB_IDOCACHE),  // Async Memory Bank 1 (Prim B)
  0x20000000, (PAGE_SIZE_1MB | CPLB_IDOCACHE),  // Async Memory Bank 0 (Prim A)

  0x00000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  // 128 MB SDRAM
  0x00400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  // 
  0x00800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  // 
  0x00C00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  // 
  0x01000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  // 
  0x01400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x01800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x01C00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //

  0x02000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x02400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x02800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x02C00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x03000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x03400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x03800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x03C00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //

  0x04000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x04400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x04800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x04C00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x05000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x05400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x05800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x05C00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //

  0x06000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x06400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x06800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x06C00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x07000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x07400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x07800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x07C00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //

  0x20300000, (PAGE_SIZE_1MB | CPLB_INOCACHE),  // Async Memory Bank 3

  0xffffffff; // end of section - termination
._icplbs_table.end:

#else

.section/DOUBLEANY cplb_data;

.align 4;

_dcplbs_table:
_icplbs_table:
  .byte4=0xffffffff;
._dcplbs_table.end:
._icplbs_table.end:

#endif

.global _icplbs_table;
.type _icplbs_table, STT_OBJECT;
.global _dcplbs_table;
.type _dcplbs_table, STT_OBJECT;
