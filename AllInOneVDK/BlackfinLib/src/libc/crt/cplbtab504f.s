/* Copyright (C) 2009 Analog Devices, Inc. All Rights Reserved. */

/* CPLB table definitions for ADSP-BF504F. */

#if defined(__ADSPBF504F__)
#include <cplb.h>

.SECTION/DOUBLEANY cplb_data;
// The possible placements of stack in memory must be covered by a locked
// DCPLB to ensure it is always available for use by exception handlers.
.ALIGN 4;
.BYTE4 _dcplbs_table [] =
  // L1 Data A SRAM (set write-through bit to avoid 1st write exceptions)
  0xFF800000, (PAGE_SIZE_4KB | CPLB_DNOCACHE | CPLB_LOCK| CPLB_WT),
  0xFF801000, (PAGE_SIZE_4KB | CPLB_DNOCACHE | CPLB_LOCK| CPLB_WT),
  0xFF802000, (PAGE_SIZE_4KB | CPLB_DNOCACHE | CPLB_LOCK| CPLB_WT),
  0xFF803000, (PAGE_SIZE_4KB | CPLB_DNOCACHE | CPLB_LOCK| CPLB_WT),

  // L1 Data A SRAM/Cache (set write-through bit to avoid 1st write exceptions)
  0xFF804000, (PAGE_SIZE_4KB | CPLB_DNOCACHE | CPLB_LOCK| CPLB_WT),
  0xFF805000, (PAGE_SIZE_4KB | CPLB_DNOCACHE | CPLB_LOCK| CPLB_WT),
  0xFF806000, (PAGE_SIZE_4KB | CPLB_DNOCACHE | CPLB_LOCK| CPLB_WT),
  0xFF807000, (PAGE_SIZE_4KB | CPLB_DNOCACHE | CPLB_LOCK| CPLB_WT),

  // SYNC Flash 4M (32M Bits)
  0x20000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),

  // 4KB Boot ROM
  0xEF000000, (PAGE_SIZE_4KB | CPLB_L1_CHBL | CPLB_VALID | CPLB_USER_RD),

  0xffffffff; // end of section - termination

.BYTE4 _icplbs_table [] =
  // Code SRAM/CACHE (16K)
  0xFFA04000, (PAGE_SIZE_4KB | CPLB_I_PAGE_MGMT), 
  0xFFA05000, (PAGE_SIZE_4KB | CPLB_I_PAGE_MGMT), 
  0xFFA06000, (PAGE_SIZE_4KB | CPLB_I_PAGE_MGMT), 
  0xFFA07000, (PAGE_SIZE_4KB | CPLB_I_PAGE_MGMT), 

  // Instruction Bank A SRAM (16K) 
  0xFFA00000, (PAGE_SIZE_4KB | CPLB_I_PAGE_MGMT), 
  0xFFA01000, (PAGE_SIZE_4KB | CPLB_I_PAGE_MGMT), 
  0xFFA02000, (PAGE_SIZE_4KB | CPLB_I_PAGE_MGMT), 
  0xFFA03000, (PAGE_SIZE_4KB | CPLB_I_PAGE_MGMT), 

  // SYNC Flash 4M (32M Bits)
  0x20000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),

  // 32KB Boot ROM
  0xEF000000, (PAGE_SIZE_4KB | CPLB_IDOCACHE),  

  0xffffffff; // end of section - termination

.GLOBAL _icplbs_table;
.TYPE _icplbs_table, STT_OBJECT;
.GLOBAL _dcplbs_table;
.TYPE _dcplbs_table, STT_OBJECT;

#else
# error Wrong target!
#endif

