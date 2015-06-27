/*
** Flush a data buffer from the cache, in case it has been modified and not
** yet been written back to memory. Optionally, invalidate it.
** void flush_data_buffer(void *start, void *end, int invalidate);
**
** Copyright (C) Analog Devices, Inc. 2004-2007
*/

.file_attr libName=libevent;
.file_attr FuncName=_flush_data_buffer;
.file_attr libFunc=_flush_data_buffer;
.file_attr libFunc=flush_data_buffer;
.file_attr prefersMem=internal;
.file_attr prefersMemNum="30";

#include <sys/anomaly_macros_rtl.h>
.section program;

.align 2;

_flush_data_buffer:
	CC = R2;
	P0 = R0;
	P1 = R1;
	P2 = -1;          // infinite loop count
	IF CC JUMP .invloop;

.respin:
	LSETUP(.fstart, .fend) LC0 = P2;
.fstart:	FLUSH[P0++];
#if WA_05000096
# if defined(__WORKAROUND_SPECULATIVE_SYNCS) 
		NOP; NOP;
#  if !defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
		NOP;
#  endif
# endif
# if defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
		CLI R0;
# endif
		CSYNC;
# if defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
		STI R0;
# endif
#endif
		CC = P0 < P1;
#if defined(__ADSPLPBLACKFIN__)
.fend: 	IF !CC JUMP .fdone;
#else
		// jump not allowed as last instr of a HW loop
		IF !CC JUMP .fdone;
.fend:  	NOP;
#endif
	JUMP .respin;

.fdone:
	/* If the data crosses a cache line, then we'll be pointing to
	** the last cache line, but won't have flushed it yet, so do
	** one more.
	*/
	FLUSH[P0];
#if WA_05000096
# if defined(__WORKAROUND_SPECULATIVE_SYNCS)
	NOP;
#  if !defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
	NOP;
#  endif
# endif
# if defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
	CLI R0;
# endif
	CSYNC;
# if defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
	STI R0;
# endif
#endif
	RTS;

.invloop:

.irespin:
	LSETUP(.ifstart, .ifend) LC0 = P2;
.ifstart:	FLUSHINV[P0++];
#if WA_05000096
# if defined(__WORKAROUND_SPECULATIVE_SYNCS)
		NOP; NOP;
#  if !defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
		NOP;
#  endif
# endif
# if defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
		CLI R0;
# endif
		CSYNC;
# if defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
		STI R0;
# endif
#endif
		CC = P0 < P1;
#if defined(__ADSPLPBLACKFIN__)
.ifend: 	IF !CC JUMP .ifdone;
#else
		// jump not allowed as last instr of a HW loop
		IF !CC JUMP .ifdone;
.ifend:  	NOP;
#endif
	JUMP .irespin;

.ifdone:
	/* If the data crosses a cache line, then we'll be pointing to
	** the last cache line, but won't have flushed it yet, so do
	** one more.
	*/
	FLUSHINV[P0];
#if WA_05000096
# if defined(__WORKAROUND_SPECULATIVE_SYNCS)
	NOP;
#  if !defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
	NOP;
#  endif
# endif
# if defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
	CLI R0;
# endif
	CSYNC;
# if defined(__WORKAROUND_SYNC_LOOP_ANOM_312)
	STI R0;
# endif
#endif
	RTS;

._flush_data_buffer.end:
.global _flush_data_buffer;
.type _flush_data_buffer, STT_FUNC;

/*
vi:ts=7
*/
