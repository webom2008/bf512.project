/************************************************************************
 *
 * btc_write.h
 *
 * These functions are used to transfer data to the VisualDSP plot
 * window via BTC.
 *
 * (c) Copyright 2005 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#include "btc.h"

.section/DOUBLEANY program;

.global _btc_write_array;
.align 8;

_btc_write_array:

//R0 contains the BTC channel number
//R1 contains the address of the data array to store
//R2 contains the size of the data array (bytes)

[--SP] = (r7:5);
[--SP] = (p5:0);
[--SP] = astat;
[--SP] = rets;

SP += -28;	

[SP+ 8] = R0;
[SP+ 12] = R1;
[SP+ 16] = R2;

r0 = [SP+ 8];
call _BTC_CHANNEL_LEN;
[SP+ 20] = p0;	//chan length

r0 = [SP+ 8];
call _BTC_CHANNEL_ADDR;
[SP+ 24] = p0;	//chan address

r1 = [SP+ 20];	//chan length
p2 = [SP+ 24];	//chan address
nop;nop;nop;

//chan big enough?
r0 = [SP+ 16];	//array size (bytes)
r1 += -12;		//chan size - header size
cc = R1 < R0;
if cc jump BtcWriteDone;

//if count not equal to 0 then data loss
r1 = 0;
r2 = p2;
p0 = r2;
nop;nop;nop; nop; //Preg read after write which requires 4 extra cycles
r0 = [p0];
cc = r0 == 0;
if cc jump no_btc_loss;
r1 = 1;
no_btc_loss:

//set count to 0 while updating
r0 = 0;
p0 = [SP+ 24];
nop;nop;nop;
[p0] = r0;

//copy data to channel
p1 = [SP+ 12];	//data address
p5 = [SP+ 16];	//data size (bytes)
nop;nop;nop;
p0 = [SP+ 24];
nop;nop;nop;
p0 += 12;	//skip header
LSETUP ( lp_start, lp_end) lc0 = p5;
lp_start:
r0 = b[p1++] (z); 
lp_end: b[p0++] = r0;

//write BTC flag, count and loss flag
r0 = [SP+ 16];	//data size
p0 = [SP+ 24];	//chan address
nop;nop;nop;

//store "BTC" for plot window verify
r2 = 0x00;	//
b[p0++] = r2;
r2 = 0x43;	//C
b[p0++] = r2;
r2 = 0x54;	//T
b[p0++] = r2;
r2 = 0x42;	//B
b[p0++] = r2;

[p0++] = r0;	//count
[p0++] = r1;	//data loss

BtcWriteDone:

SP +=  28;

// Restore Regs
rets = [SP++];
astat = [SP++];
(p5:0) = [SP++];  		
(r7:5) = [SP++];

rts;
nop;

_btc_write_array.end:

/************************************************************************/

.global _btc_write_value;
.align 8;

_btc_write_value:

//R0 contains the BTC channel number
//R1 contains the data value to store
//R2 contains the size of the data (bytes)

[--SP] = (r7:0);
[--SP] = (p5:0);
[--SP] = astat;
[--SP] = rets;

SP += -28;	//allocate space for local variables

[SP+ 8] = R0;
[SP+ 12] = R1;
[SP+ 16] = R2;

r0 = [SP+ 8];
call _BTC_CHANNEL_LEN;
[SP+ 20] = p0;	//chan length

r0 = [SP+ 8];
call _BTC_CHANNEL_ADDR;
[SP+ 24] = p0;	//chan address

r1 = [SP+ 20];	//chan length
p2 = [SP+ 24];	//chan address
nop;nop;nop;

p0 = p2;	
p0 += 4;	//skip BTC flag (4 bytes)
nop;nop;nop;

//get chan count
r1 = [p0];

//set count to 0 while updating
r0 = 0;
[p0] = r0;
p0 += 8; //skip header

//chan exceeded?
r2 = 0;
r0 = [SP+ 20];	//chan size (bytes)
r0 += -12;		//chan size - header size
cc = r1 < r0;
if cc jump btc_chan_ok;
//chan full, reset count, set data loss flag
r1 = 0;
r2 = 1;
btc_chan_ok:

p1 = r1;
nop;nop;nop; nop; //Preg read after write which requires 4 extra cycles
p0 = p0 + p1;	//move ptr 

//copy data to channel
p1 = [SP+ 12];	//data address
p5 = [SP+ 16];	//data size (bytes)
nop;nop;nop;

LSETUP ( lp_start1, lp_end1) lc0 = p5;
lp_start1:
r0 = b[p1++] (z); 
lp_end1: b[p0++] = r0;

r0 = p5;
r1 = r1 + r0;	//bytes

//store "BTC" for plot window verify
r0 = 0x00;	//
b[p2++] = r0;
r0 = 0x43;	//C
b[p2++] = r0;
r0 = 0x54;	//T
b[p2++] = r0;
r0 = 0x42;	//B
b[p2++] = r0;

[p2] = r1;	//store count

//test for data loss
cc = r2 == 1;	//test for data loss
if !cc jump btc_no_data_loss1;
p2 += 4;		
[p2] = r2;		//store data loss flag
btc_no_data_loss1:

SP +=  28;

// Restore Regs
rets = [SP++];
astat = [SP++];
(p5:0) = [SP++];  		
(r7:0) = [SP++];

rts;
nop;

_btc_write_value.end:

