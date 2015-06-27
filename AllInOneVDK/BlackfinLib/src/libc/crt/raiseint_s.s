/* Copyright (C) 2000-2009 Analog Devices Inc., All Rights Reserved.
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
*/
/*
** A user-level convenience function for triggering exceptions and
** interrupts.
*/
.file_attr libName=libsmall;
.file_attr FuncName=_raise_interrupt;
.file_attr libFunc=_raise_interrupt;
.file_attr libFunc=raise_interrupt;
.file_attr prefersMem=internal;
.file_attr prefersMemNum="30";

#include <sys/excause.h>
#define EXCEPTION 3
/*
** int raise_interrupt(
**   interrupt_kind kind,
**   int which, int cmd, int arg1, int arg2)
*/

.section program;
.align 2;

_raise_interrupt:
	P0.L = .int_jump_table;
	P0.H = .int_jump_table;
	CC = R0 == 3;
	IF !CC JUMP .not_exception;
	P0.L = .exc_jump_table;
	P0.H = .exc_jump_table;
	R0 = R1;
.not_exception:
	CC = R0 < 0;
	IF CC JUMP .bad_val;
	R1 = 15;
	CC = R0 <= R1;
	IF !CC JUMP .bad_val;
	P1 = R0;
	P1 = P1 << 2;
	P0 = P0 + P1;
	R0 = R2;	// which	
	R1 = [SP +12];	// arg1
	R2 = [SP +16];	// arg2
	JUMP (P0);
.bad_val:
	R0 = -1 (X);
	RTS;

.int_jump_table:
	EMUEXCPT;	RTS;	// Emulation exception
	RAISE 1; 	RTS;	// Reset - shouldn't happen
	RAISE 2;	RTS;	// NMI
	NOP;		RTS;	// Exception - handled below
	NOP;		RTS;	// global int disable - not used
	RAISE 5;	RTS;	// Hardware fault
	RAISE 6;	RTS;	// Timer
	RAISE 7;	RTS;	// User interrupts
	RAISE 8;	RTS;
	RAISE 9;	RTS;
	RAISE 10;	RTS;
	RAISE 11;	RTS;
	RAISE 12;	RTS;
	RAISE 13;	RTS;
	RAISE 14;	RTS;
	RAISE 15;	RTS;

.exc_jump_table:
	EXCPT 0;	RTS;
	EXCPT 1;	RTS;
	EXCPT 2;	RTS;
	EXCPT 3;	RTS;
	EXCPT 4;	RTS;
	EXCPT 5;	RTS;
	EXCPT 6;	RTS;
	EXCPT 7;	RTS;
	EXCPT 8;	RTS;
	EXCPT 9;	RTS;
	EXCPT 10;	RTS;
	EXCPT 11;	RTS;
	EXCPT 12;	RTS;
	EXCPT 13;	RTS;
	EXCPT 14;	RTS;
	EXCPT 15;	RTS;

._raise_interrupt.end:
.global _raise_interrupt;
.type _raise_interrupt, STT_FUNC;
