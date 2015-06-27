/************************************************************************
 *
 * btc_driver.h
 *
 * (c) Copyright 2003 Analog Devices, Inc.  All rights reserved.
 *
 *
 *  This file contains Background Telemetry Channel (BTC)
 *  function implementations.
 *
 ************************************************************************/

.extern __btc_map;
.extern __btc_nNumBtcMapEntries;
.import "btc_struct.h";

#if defined (__ADSPBF535__) || defined(__AD6532__)
	#define EMUDAT_WR	cli r0; ssync; nop; [p4] = r5; ssync; nop; sti r0
	#define EMUDAT_RD	cli r0; ssync; nop; r5 = [p4]; ssync; nop; sti r0
#elif defined (__ADSPLPBLACKFIN__)
	#define EMUDAT_WR 	cli r0; ssync; nop; emudat = r5; ssync; nop; sti r0
	#define EMUDAT_RD	cli r0; ssync; nop; r5 = emudat; ssync; nop; sti r0
#else
	#error "This part does not support BTC"
#endif

.section/DOUBLEANY program;

.global _BTC_CHANNEL_ADDR;
_BTC_CHANNEL_ADDR:
		// channel index expected in R0
		// P0 contains channel address when done
		[--sp] = r0;
		[--sp] = r1;
		[--sp] = a0.w;
		[--sp] = a0.x;
		[--sp] = a1.w;
		[--sp] = a1.x;
		r1 = sizeof(_BTC_MAP_DATA);
		r1 *= r0;
		r0.h = __btc_map + offsetof(_BTC_MAP_DATA, address);
		r0.l = __btc_map + offsetof(_BTC_MAP_DATA, address);
		r0 = r0 + r1;
		p0 = r0;
		nop;nop;nop; nop; //Preg read after write which requires 4 extra cycles
		r0 = [p0];
		p0 = r0;
		a1.x = [sp++];
		a1.w = [sp++];
		a0.x = [sp++];
		a0.w = [sp++];
		r1 = [sp++];
		r0 = [sp++];
		rts;
_BTC_CHANNEL_ADDR.end:		

.global _BTC_CHANNEL_LEN;
_BTC_CHANNEL_LEN:
		// channel index expected in R0
		// P0 contains channel length when done
		[--sp] = r0;
		[--sp] = r1;
		[--sp] = a0.w;
		[--sp] = a0.x;
		[--sp] = a1.w;
		[--sp] = a1.x;
		r1 = sizeof(_BTC_MAP_DATA);
		r1 *= r0;
		r0.h = __btc_map + offsetof(_BTC_MAP_DATA, length);
		r0.l = __btc_map + offsetof(_BTC_MAP_DATA, length);
		r0 = r0 + r1;
		p0 = r0;
		nop;nop;nop; nop; //Preg read after write which requires 4 extra cycles
		r0 = [p0];
		p0 = r0;
		a1.x = [sp++];
		a1.w = [sp++];
		a0.x = [sp++];
		a0.w = [sp++];
		r1 = [sp++];
		r0 = [sp++];
		rts;
_BTC_CHANNEL_LEN.end:		


////////////////////////
//	Assembly version
////////////////////////
.global _btc_init;
_btc_init:
		[--sp] = p3;
		[--sp] = r5;
		
		// p3 points to DBGSTAT
		p3.h = 0xffe0;
		p3.l = 0x5008;
		r5 = 0;
		// clear any flags in DBGSTAT
		ssync;
		nop; 
		[p3] = r5;
		ssync;
		nop; 
		
		r5 = [sp++];
		p3 = [sp++];
		rts;
_btc_init.end:		

///////////////////////////
//	Assembly version
///////////////////////////
.global _btc_poll;
_btc_poll:
		[--sp] = p3;
		[--sp] = p4;
		[--sp] = r5;
		[--sp] = r0;
		[--sp] = astat;
		
		// p3 points to DBGSTAT
		p3.h = 0xffe0;
		p3.l = 0x5008;
#if defined (__ADSPBF535__) || defined(__AD6532__)
		// p4 points to EMUDAT
		p4.h = 0xffe0;
		p4.l = 0x500c;
#endif		

		ssync;
		nop; 
		r5 = [p3];					// read DBGSTAT
		ssync;
		nop; 
		
		nop;
		cc = bittst(r5, 1);			// check for incoming cmd
		if !cc jump EXIT_POLL;
		[--sp] = rets;
		call INCOMING_CMD;
		rets = [sp++];

EXIT_POLL:
		//sti r0;		// reenable interrupts
		astat = [sp++];
		r0 = [sp++];
		r5 = [sp++];
		p4 = [sp++];
		p3 = [sp++];
		rts;
_btc_poll.end:		

// incoming commands will have the following format:
//		bit 0:		Read Byte command bit
//		bit 1:  	Write Byte command bit
// 		bit 2:		Read Word command bit
//		bit 3:		Write Word command bit
//		bit 4:		Read DWord command bit
//		bit 5:		Write DWord command bit
//		bit 6:		Query commnad bit
//
//	r5, p3, and p4 are already pushed by POLL_BTC!!
INCOMING_CMD:
		
		EMUDAT_RD;					// read EMUDAT
		cc = bittst(r5, 0);			// check for a Host Read Byte (8-bits) command
		if cc jump _BTC_READ_BYTE;
		cc = bittst(r5, 1);			// check for a Host Write Byte (8-bits) command
		if cc jump _BTC_WRITE_BYTE;
		cc = bittst(r5, 2);			// check for a Host Read Word (16-bits) command
		if cc jump _BTC_READ_WORD;
		cc = bittst(r5, 3);			// check for a Host Write Word (16-bits) command
		if cc jump _BTC_WRITE_WORD;
		cc = bittst(r5, 4);			// check for a Host Read DWord (32-bits) command
		if cc jump _BTC_READ_DWORD;
		cc = bittst(r5, 5);			// check for a Host Write DWord (32-bits) command
		if cc jump _BTC_WRITE_DWORD;
		cc = bittst(r5, 6);			// check for a Query command
		if cc jump _BTC_QUERY;
BAD_BTC_CMD:
		nop;
		rts;
		
BTC_CMD_COMPLETED:
		nop;
		rts;

/////////////////////////////////////////////////////////////////////////
//  Query
//
// process a Query command sent by the host.  Send back the number of
// channels defined and then the parameters for each individual channel
//
//	r5, p3, and p4 are already pushed by POLL_BTC!!
_BTC_QUERY:
		[--sp] = i0;
		[--sp] = l0;
		[--sp] = r6;
		[--sp] = lc0;
		[--sp] = lt0;
		[--sp] = lb0;
		[--sp] = lc1;
		[--sp] = lt1;
		[--sp] = lb1;
		
		l0 = 0 (z);

		i0.h = __btc_nNumBtcMapEntries;
		i0.l = __btc_nNumBtcMapEntries;
		r5 = [i0];

		EMUDAT_WR;					// write nNumBtcMapEntries to EMUDAT

		r6 = 10 (z);				// 10 is the number of words of data
									// for each individual channel
		i0.h = __btc_map;
		i0.l = __btc_map;

		lc0 = r5;					// loop on each channel
		nop;nop;nop;nop;nop;nop;nop;nop;nop; //Sysreg write after write which requires 9 extra cycles
		
		loop SEND_CHANNEL lc0;
		loop_begin SEND_CHANNEL;
			nop;
			lc1 = r6;						// loop on each word of data within a channel
			nop;nop;nop;nop;nop;nop;nop;nop;nop; //Sysreg write after write which requires 9 extra cycles
			
			loop SEND_CHANNEL_INFO lc1;
			loop_begin SEND_CHANNEL_INFO;
				[--sp] = rets;
				call DOF_WAIT;				// wait for EMUDAT to be come empty
				rets = [sp++];
				r5 = [i0++];	
				EMUDAT_WR;					// write next piece of data to EMUDAT
				nop;
			loop_end SEND_CHANNEL_INFO;
		loop_end SEND_CHANNEL;
		
		lb1 = [sp++];
		lt1 = [sp++];
		lc1 = [sp++];
		lb0 = [sp++];
		lt0 = [sp++];
		lc0 = [sp++];
		r6  = [sp++];
		l0  = [sp++];
		i0  = [sp++];
		nop;
		jump BTC_CMD_COMPLETED;

/////////////////////////////////////////////////////////////////////////
//  Read
//
// process a Read command sent by the host.  Send back the data associated
// with the particular channel
//
//	r5, p3, and p4 are already pushed by POLL_BTC!!
_BTC_READ_BYTE:
		[--sp] = p5;
		[--sp] = lc0;
		[--sp] = lt0;
		[--sp] = lb0;
		
		[--sp] = rets;
		call DIF_WAIT;
		rets = [sp++];
		EMUDAT_RD;				// wait for host to send address
		p5 = r5;

		[--sp] = rets;
		call DIF_WAIT;
		rets = [sp++];
		EMUDAT_RD;				// wait for host to send length

		lc0 = r5;
		nop;nop;nop;nop;nop;nop;nop;nop;nop; //Sysreg write after write which requires 9 extra cycles
		loop SEND_BYTE_BLOCK lc0;
		loop_begin SEND_BYTE_BLOCK;
			[--sp] = rets;
			call DOF_WAIT;
			rets = [sp++];
			r5 = b[p5++] (z);
			EMUDAT_WR;
		loop_end SEND_BYTE_BLOCK;

		lb0 = [sp++];
		lt0 = [sp++];
		lc0 = [sp++];
		p5  = [sp++];
		jump BTC_CMD_COMPLETED;


_BTC_READ_WORD:
		[--sp] = p5;
		[--sp] = lc0;
		[--sp] = lt0;
		[--sp] = lb0;

		[--sp] = rets;
		call DIF_WAIT;
		rets = [sp++];
		EMUDAT_RD;				// wait for host to send address
		p5 = r5;

		[--sp] = rets;
		call DIF_WAIT;
		rets = [sp++];
		EMUDAT_RD;				// wait for host to send length
		lc0 = r5;
		nop;nop;nop;nop;nop;nop;nop;nop;nop; //Sysreg write after write which requires 9 extra cycles
		loop SEND_WORD_BLOCK lc0;
		loop_begin SEND_WORD_BLOCK;
			[--sp] = rets;
			call DOF_WAIT;
			rets = [sp++];
			r5 = w[p5++] (z);
			EMUDAT_WR;
		loop_end SEND_WORD_BLOCK;

		lb0 = [sp++];
		lt0 = [sp++];
		lc0 = [sp++];
		p5  = [sp++];
		jump BTC_CMD_COMPLETED;


_BTC_READ_DWORD:
		[--sp] = i0;
		[--sp] = l0;
		[--sp] = lc0;
		[--sp] = lt0;
		[--sp] = lb0;
		
		l0 = 0 (z);

		[--sp] = rets;
		call DIF_WAIT;
		rets = [sp++];
		EMUDAT_RD;				// wait for host to send address
		i0 = r5;

		[--sp] = rets;
		call DIF_WAIT;
		rets = [sp++];
		EMUDAT_RD;				// wait for host to send length
		lc0 = r5;
		nop;nop;nop;nop;nop;nop;nop;nop;nop; //Sysreg write after write which requires 9 extra cycles
		
		loop SEND_DWORD_BLOCK lc0;
		loop_begin SEND_DWORD_BLOCK;
			[--sp] = rets;
			call DOF_WAIT;
			rets = [sp++];
			r5 = [i0++];
			EMUDAT_WR;
		loop_end SEND_DWORD_BLOCK;

		lb0 = [sp++];
		lt0 = [sp++];
		lc0 = [sp++];
		l0  = [sp++];
		i0  = [sp++];
		jump BTC_CMD_COMPLETED;

/////////////////////////////////////////////////////////////////////////
//  Write
//
// process a Write command sent by the host.  Host will send the address to
// write to followed by the length then the data
//
//	r5, p3, and p4 are already pushed by POLL_BTC!!
_BTC_WRITE_BYTE:
		[--sp] = p5;
		[--sp] = lc0;
		[--sp] = lt0;
		[--sp] = lb0;

		[--sp] = rets;
		call DIF_WAIT;
		rets = [sp++];
		EMUDAT_RD;				// wait for host to send address
		p5 = r5;

		[--sp] = rets;
		call DIF_WAIT;
		rets = [sp++];
		EMUDAT_RD;				// wait for host to send length
		lc0 = r5;
		nop;nop;nop;nop;nop;nop;nop;nop;nop; //Sysreg write after write which requires 9 extra cycles
		
		loop RCV_BYTE_BLOCK lc0;
		loop_begin RCV_BYTE_BLOCK;
			[--sp] = rets;
			call DIF_WAIT;
			rets = [sp++];
			EMUDAT_RD;			// data to write to the DSP
			b[p5++] = r5;
		loop_end RCV_BYTE_BLOCK;

		lb0 = [sp++];
		lt0 = [sp++];
		lc0 = [sp++];
		p5  = [sp++];
		jump BTC_CMD_COMPLETED;


_BTC_WRITE_WORD:
		[--sp] = p5;
		[--sp] = lc0;
		[--sp] = lt0;
		[--sp] = lb0;

		[--sp] = rets;
		call DIF_WAIT;
		rets = [sp++];
		EMUDAT_RD;				// wait for host to send address
		p5 = r5;

		[--sp] = rets;
		call DIF_WAIT;
		rets = [sp++];
		EMUDAT_RD;				// wait for host to send length
		lc0 = r5;
		nop;nop;nop;nop;nop;nop;nop;nop;nop; //Sysreg write after write which requires 9 extra cycles
		
		loop RCV_WORD_BLOCK lc0;
		loop_begin RCV_WORD_BLOCK;
			[--sp] = rets;
			call DIF_WAIT;
			rets = [sp++];
			EMUDAT_RD;
			w[p5++] = r5;
		loop_end RCV_WORD_BLOCK;

		lb0 = [sp++];
		lt0 = [sp++];
		lc0 = [sp++];
		p5  = [sp++];
		jump BTC_CMD_COMPLETED;


_BTC_WRITE_DWORD:
		[--sp] = i0;
		[--sp] = l0;
		[--sp] = lc0;
		[--sp] = lt0;
		[--sp] = lb0;
		
		l0 = 0 (z);
	
		[--sp] = rets;
		call DIF_WAIT;
		rets = [sp++];
		EMUDAT_RD;				// wait for host to send address
		i0 = r5;

		[--sp] = rets;
		call DIF_WAIT;
		rets = [sp++];
		EMUDAT_RD;				// wait for host to send length
		lc0 = r5;
		nop;nop;nop;nop;nop;nop;nop;nop;nop; //Sysreg write after write which requires 9 extra cycles
		
		loop RCV_DWORD_BLOCK lc0;
		loop_begin RCV_DWORD_BLOCK;
			[--sp] = rets;
			call DIF_WAIT;
			rets = [sp++];
			EMUDAT_RD;
			[i0++] = r5;
		loop_end RCV_DWORD_BLOCK;

		lb0 = [sp++];
		lt0 = [sp++];
		lc0 = [sp++];
		l0  = [sp++];
		i0  = [sp++];
		jump BTC_CMD_COMPLETED;


/////////////////////////////////////////////////////////////////////////
//  DOF Wait
//
//  a loop that polls the DBGSTAT register until EMUDAT_OUT becomes empty
// 
//	r5, p3, and p4 are already pushed by POLL_BTC!!
DOF_WAIT:
		ssync;
		nop; 
		r5 = [p3];				// read DBGSTAT
		ssync;
		nop;
		cc = bittst(r5, 0);		// check the EMUDOF flag
		if cc jump DOF_WAIT;		
		nop;
		rts;

/////////////////////////////////////////////////////////////////////////
//  DIF Wait
//
//  a loop that polls the DBGSTAT register until EMUDAT_IN becomes full
// 
//	r5, p3, and p4 are already pushed by POLL_BTC!!
DIF_WAIT:
		ssync;
		nop; 
		r5 = [p3];				// read DBGSTAT
		ssync;
		nop;
		cc = bittst(r5, 1);		// check the EMUDIF flag
		if !cc jump DIF_WAIT;		
		nop;
		rts;

		
