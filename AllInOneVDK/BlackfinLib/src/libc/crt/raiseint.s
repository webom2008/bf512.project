/* Copyright (C) 2000-2008 Analog Devices Inc., All Rights Reserved.
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
*/
/*
** A user-level convenience function for triggering exceptions and
** interrupts.
*/

#if !defined(__NO_LIBRARY_ATTRIBUTES__)
.file_attr libName=libevent;
.file_attr FuncName=_raise_interrupt;
.file_attr libFunc=_raise_interrupt;
.file_attr libFunc=raise_interrupt;
.file_attr prefersMem=internal;
.file_attr prefersMemNum="30";
#endif

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
	R3 = EXCEPTION;
	CC = R0 == R3;		// Is this an exception?
	IF CC JUMP .is_exception;

	// Interrupts can't be raised in user mode, so we have to invoke a
	// system service to ask for one.

	LINK 20;		// five params
	[SP+ 16] = R1;		// push which
	[SP+ 12] = R0;		// push kind
	R2 = EX_SYSREQ_RAISE_INT;
	R1 = EX_SYS_REQ;
	R0 = R3;		// asking for exception
	CALL.X _raise_interrupt;
	UNLINK;
	RTS;

.is_exception:			// so check it's a valid one

	R3 = 0;
	CC = R1 < R3;
	IF CC JUMP .wrong;
	R3 = 15;
	CC = R3 < R1;
	IF CC JUMP .wrong;

	// load up the parameters

	P1 = R1;		// which exception we want
	R0 = R2;		// the command to be issued
	P0.L = .excpts;		// start of jump table
	P0.H = .excpts;
	R1 = [SP +12];		// arg1
	R2 = [SP +16];		// arg2
	P0 = P0 + (P1<<2);	// ex num * 32 bits
	JUMP (P0);
.excpts:
	EXCPT 0; RTS;
	EXCPT 1; RTS;
	EXCPT 2; RTS;
	EXCPT 3; RTS;
	EXCPT 4; RTS;
	EXCPT 5; RTS;
	EXCPT 6; RTS;
	EXCPT 7; RTS;
	EXCPT 8; RTS;
	EXCPT 9; RTS;
	EXCPT 10; RTS;
	EXCPT 11; RTS;
	EXCPT 12; RTS;
	EXCPT 13; RTS;
	EXCPT 14; RTS;
	EXCPT 15; RTS;
.wrong:
	R0 = -1;
	RTS;
._raise_interrupt.end:

.global _raise_interrupt;
.type _raise_interrupt,STT_FUNC;

