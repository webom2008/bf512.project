/* Copyright (C) 2000-2009 Analog Devices Inc., All Rights Reserved,
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
*/

/*
** _Exit() system call (lower level than the exit() library call).
** Invoke an exception handler to halt the
** processor. This function should never return.
** Note that the ___lib_prog_term symbol is present because the ADI
** debuggers automatically set a breakpoint on it. This allows the
** debugger to halt after stdio has flushed, rather than on entry to
** exit.
*/
.file_attr libName=libc;
.file_attr FuncName=__Exit;
.file_attr FuncName=___lib_prog_term;
.file_attr libFunc=__Exit;
.file_attr libFunc=___lib_prog_term;
.file_attr libFunc=exit;
.file_attr libFunc=_exit;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";

	.section program;
	.align 2;

__Exit:
		R0=0;
___lib_prog_term:		// Debugger will halt here
		EXCPT 0;			// enter exception handler and do HLT
		JUMP ___lib_prog_term;
.___lib_prog_term.end:
.__Exit.end:

	.global __Exit;
	.type __Exit,STT_FUNC;
	.global ___lib_prog_term;
	.type ___lib_prog_term,STT_FUNC;
