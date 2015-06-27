/* Copyright (C) 2006-2009 Analog Devices, Inc. All Rights Reserved. */

/* CPLB table definitions for ADSP-BF549. */

#if defined(__ADSPBF549__)
#include <cplb.h>

.SECTION/DOUBLEANY cplb_data;
// The possible placements of stack in memory must be covered by a locked
// DCPLB to ensure it is always available for use by exception handlers.
.ALIGN 4;
_dcplbs_table:
.BYTE4=
  // L1 Data A & B, (set write-through bit to avoid 1st write exceptions)
  0xFF800000, (PAGE_SIZE_4MB | CPLB_DNOCACHE | CPLB_LOCK| CPLB_WT),

  // L2 SRAM - possible stack use so locked.
  0xFEB00000, (PAGE_SIZE_1MB | CPLB_DDOCACHE | CPLB_LOCK), 

  // 512 MB DDR1 memory space
  // MEM_SDRAM0_BANK0 (16MB - defined in default LDF) potentially utilised as
  // stack, therefore locked.
  0x00000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE | CPLB_LOCK),
  0x00400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE | CPLB_LOCK),
  0x00800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE | CPLB_LOCK),
  0x00C00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE | CPLB_LOCK),
  0x01000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE), //
  0x01400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE), //
  0x01800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE), //
  0x01C00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE), //

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

  0x08000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x08400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x08800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x08C00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x09000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x09400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x09800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x09C00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //

  0x0a000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x0a400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x0a800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x0aC00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x0b000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x0b400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x0b800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x0bC00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //

  0x0c000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x0c400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x0c800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x0cC00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x0d000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x0d400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x0d800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x0dC00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //

  0x0e000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x0e400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x0e800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x0eC00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x0f000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x0f400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x0f800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x0fC00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //

  0x10000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x10400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x10800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x10C00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x11000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x11400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x11800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x11C00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //

  0x12000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x12400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x12800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x12C00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x13000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x13400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x13800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x13C00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //

  0x14000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x14400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x14800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x14C00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x15000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x15400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x15800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x15C00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //

  0x16000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x16400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x16800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x16C00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x17000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x17400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x17800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x17C00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //

  0x18000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x18400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x18800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x18C00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x19000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x19400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x19800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x19C00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //

  0x1a000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x1a400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x1a800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x1aC00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x1b000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x1b400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x1b800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x1bC00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //

  0x1c000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x1c400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x1c800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x1cC00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x1d000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x1d400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x1d800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x1dC00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //

  0x1e000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x1e400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x1e800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x1eC00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x1f000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x1f400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x1f800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //
  0x1fC00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  //

  0x2FC00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // Async Mem Bank 3 (64MB)
  0x2F800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x2F400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x2F000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x2EC00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x2E800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x2E400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x2E000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x2DC00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x2D800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x2D400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x2D000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x2CC00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x2C800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x2C400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x2C000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 

  0x2BC00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // Async Mem Bank 2 (64MB)
  0x2B800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x2B400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x2B000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x2AC00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x2A800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x2A400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x2A000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x29C00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x29800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x29400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x29000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x28C00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x28800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x28400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x28000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 

  0x27C00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // Async Mem Bank 1 (64MB)
  0x27800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x27400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x27000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x26C00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x26800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x26400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x26000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x25C00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x25800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x25400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x25000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x24C00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x24800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x24400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x24000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 

  0x23C00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // Async Mem Bank 0 (64MB)
  0x23800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x23400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x23000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x22C00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x22800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x22400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x22000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x21C00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x21800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x21400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x21000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x20C00000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x20800000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x20400000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 
  0x20000000, (PAGE_SIZE_4MB | CPLB_DDOCACHE),  // 

                                                // 4KB Boot ROM
  0xEF000000, (PAGE_SIZE_4KB | CPLB_L1_CHBL | CPLB_VALID | CPLB_USER_RD),

  0xffffffff; // end of section - termination 
._dcplbs_table.end: 

_icplbs_table:
.byte4=
  0xFFA00000, (PAGE_SIZE_1MB | CPLB_I_PAGE_MGMT), // L1 Code

  0xFEB00000, (PAGE_SIZE_1MB | CPLB_IDOCACHE),  // L2

  0x00000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  // 512 MB DDR1 memory space
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

  0x08000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x08400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x08800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x08C00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x09000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x09400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x09800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x09C00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //

  0x0a000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x0a400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x0a800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x0aC00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x0b000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x0b400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x0b800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x0bC00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //

  0x0c000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x0c400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x0c800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x0cC00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x0d000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x0d400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x0d800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x0dC00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //

  0x0e000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x0e400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x0e800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x0eC00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x0f000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x0f400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x0f800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x0fC00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //

  0x10000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x10400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x10800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x10C00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x11000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x11400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x11800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x11C00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //

  0x12000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x12400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x12800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x12C00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x13000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x13400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x13800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x13C00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //

  0x14000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x14400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x14800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x14C00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x15000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x15400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x15800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x15C00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //

  0x16000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x16400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x16800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x16C00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x17000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x17400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x17800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x17C00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //

  0x18000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x18400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x18800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x18C00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x19000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x19400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x19800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x19C00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //

  0x1a000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x1a400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x1a800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x1aC00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x1b000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x1b400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x1b800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x1bC00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //

  0x1c000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x1c400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x1c800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x1cC00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x1d000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x1d400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x1d800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x1dC00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //

  0x1e000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x1e400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x1e800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x1eC00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x1f000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x1f400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x1f800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //
  0x1fC00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),  //

  0x2FC00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  Async Mem Bank 3 (64MB)
  0x2F800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x2F400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x2F000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x2EC00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x2E800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x2E400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x2E000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x2DC00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x2D800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x2D400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x2D000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x2CC00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x2C800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x2C400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x2C000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  

  0x2BC00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  Async Mem Bank 2 (64MB)
  0x2B800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x2B400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x2B000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x2AC00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x2A800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x2A400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x2A000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x29C00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x29800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x29400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x29000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x28C00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x28800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x28400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x28000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  

  0x27C00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  Async Mem Bank 1 (64MB)
  0x27800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x27400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x27000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x26C00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x26800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x26400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x26000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x25C00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x25800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x25400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x25000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x24C00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x24800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x24400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x24000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  

  0x23C00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  Async Mem Bank 0 (64MB)
  0x23800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x23400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x23000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x22C00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x22800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x22400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x22000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x21C00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x21800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x21400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x21000000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x20C00000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x20800000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x20400000, (PAGE_SIZE_4MB | CPLB_IDOCACHE),	 //  
  0x20300000, (PAGE_SIZE_1MB | CPLB_IDOCACHE),	 //  
  0x20200000, (PAGE_SIZE_1MB | CPLB_IDOCACHE),	 //  
  0x20100000, (PAGE_SIZE_1MB | CPLB_IDOCACHE),	 //  

  0xEF000000, (PAGE_SIZE_4KB | CPLB_IDOCACHE),  // 4KB Boot ROM

  0xffffffff; // end of section - termination
._icplbs_table.end:

.GLOBAL _icplbs_table;
.TYPE _icplbs_table, STT_OBJECT;
.GLOBAL _dcplbs_table;
.TYPE _dcplbs_table, STT_OBJECT;

#else
# error Wrong target!
#endif

