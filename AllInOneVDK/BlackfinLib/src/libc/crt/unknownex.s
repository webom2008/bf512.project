/*
** Unknown-exception handler.
** Copyright (C) 2000-2007 Analog Devices, Inc. All Rights Reserved.
*/
.file_attr libName=libevent;
.file_attr FuncName=__unknown_exception_occurred;
.file_attr libFunc=__unknown_exception_occurred;
.file_attr prefersMem=any;
.file_attr prefersMemNum="50";

#include <sys/fatal_error_code.h>
.extern _adi_fatal_exception;

.section program;
.file_attr requiredForROMBoot;
.align 2;

__unknown_exception_occurred:
	// This function is invoked by the default exception
	// handler, if it does not recognise the kind of
	// exception that has occurred. In other words, the
	// default handler only handles some of the system's
	// exception types, and it does not expect any others
	// to occur. If your application is going to be using
	// other kinds of exceptions, you must replace the
	// default handler with your own, that handles all the
	// exceptions you will use.
	//
	// Since there's nothing we can do, we just call 
	// _adi_fatal_exception with the exception code (EXCAUSE).

        R0 = _AFE_G_UnhandledException;
        R1 = SEQSTAT;
        R1 = R1 << 26;    // Extract bits 0-5
        R1 = R1 >> 26;    // To give us EXCAUSE
        R2 = 0;
        JUMP.X _adi_fatal_exception;

.__unknown_exception_occurred.end:
.global __unknown_exception_occurred;
.type __unknown_exception_occurred, STT_FUNC;
