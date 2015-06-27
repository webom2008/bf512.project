/* xstdio.h internal header */
#if defined (_ADI_COMPILER)
/************************************************************************
 *
 * xstdio.h
 *
 * (c) Copyright 2001-2010 Analog Devices, Inc.  All rights reserved.
 *
 * Copyright (c) 1992-2008 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 *
 ************************************************************************/

#pragma once
#ifndef __NO_BUILTIN
#pragma system_header /* xstdio.h */
#endif
#endif /* _ADI_COMPILER */

#ifndef _XSTDIO
#define _XSTDIO
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>

#if defined (_ADI_COMPILER)
#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_12_12:"ADI header uses float within union")
#pragma diag(suppress:misra_rule_5_2:"ADI header will re-use identifiers")
#pragma diag(suppress:misra_rule_5_3:"ADI header will re-use identifiers")
#pragma diag(suppress:misra_rule_5_4:"ADI header will re-use identifiers")
#pragma diag(suppress:misra_rule_5_5:"ADI header will re-use identifiers")
#pragma diag(suppress:misra_rule_5_6:"ADI header will re-use identifiers")
#pragma diag(suppress:misra_rule_5_7:"ADI header will re-use identifiers")
#pragma diag(suppress:misra_rule_6_3:"ADI header allows use of basic types")
#pragma diag(suppress:misra_rule_8_1)
#pragma diag(suppress:misra_rule_8_5)
#pragma diag(suppress:misra_rule_12_12:"ADI header use of float within a union")
#pragma diag(suppress:misra_rule_16_3:"ADI header, identifiers not needed on all parameters")
#pragma diag(suppress:misra_rule_16_7)
#pragma diag(suppress:misra_rule_18_4:"ADI header requires union")
#pragma diag(suppress:misra_rule_19_4:"ADI header allows any substitution")
#pragma diag(suppress:misra_rule_19_7:"ADI header allows function macros")
#pragma diag(suppress:misra_rule_20_2:"ADI standard header file")
#pragma diag(suppress:misra_rule_20_9:"ADI standard header file")
#endif /* _MISRA_RULES */
# include <stdio.h>
# include "xsync.h"
# include "xatexit.h"
#else
# include <stdio.h>
#endif /* _ADI_COMPILER */

 #if _HAS_C9X
#include <stdint.h>
 #endif /* _IS_C9X */

#ifndef _YVALS
 #include <yvals.h>
#endif /* _YVALS */

_C_STD_BEGIN

		/* bits for _Mode in FILE */
#define _MOPENR	0x1
#define _MOPENW	0x2
#define _MOPENA	0x4
#define _MTRUNC	0x8
#define _MCREAT	0x10
#define _MBIN	0x20
#define _MEXCL	0x40	/* open only */

#define _MALBUF	0x40
#define _MALFIL	0x80
#define _MEOF	0x100
#define _MERR	0x200
#define _MLBF	0x400
#define _MNBF	0x800
#define _MREAD	0x1000
#define _MWRITE	0x2000
#define _MBYTE	0x4000
#define _MWIDE	0x8000

		/* codes for _Printf and _Scanf */
#define _FSP	0x01
#define _FPL	0x02
#define _FMI	0x04
#define _FNO	0x08
#define _FZE	0x10
#define _WMAX	(INT_MAX-9/10)

		/* macros for _Scanf and friends */
#define GET(px)	\
	(++(px)->nchar, (*(px)->pfn)((px)->arg, 0, 1))
#define GETN(px)	(0 <= --(px)->nget ? GET(px) \
	: (++(px)->nchar, EOF))
#define UNGET(px, ch)	\
	(--(px)->nchar, (*(px)->pfn)((px)->arg, (ch), 0))
#define UNGETN(px, ch)	do { if ((int)(ch) != EOF) { UNGET((px), (ch)); }\
	else {--(px)->nchar;} } while (0)

		/* macros for atomic file locking */

#if defined (_ADI_COMPILER)
/* Changes for new locking infrastructure. These replace the macro 
 * definitions in the Dinkumware origninal source.
 */
#pragma inline
static void
_Lockfileatomic(_Filet *_file)
	{
#if _MULTI_THREAD && _FILE_OP_LOCKS
	_CHECK_AND_ACQUIRE_MUTEX((void *)&_file->_Mutex, true);
#else
	_Locksyslock(_LOCK_STREAM);
#endif
	}

#pragma inline
static void
_Unlockfileatomic(_Filet *_file)
	{
#if _MULTI_THREAD && _FILE_OP_LOCKS
	_RELEASE_MUTEX((void *)&_file->_Mutex);
#else
	_Unlocksyslock(_LOCK_STREAM);
#endif
	}
#endif /* _ADI_COMPILER */

		/* type definitions */
typedef struct
	{	/* print formatting information */
	union
		{	/* long or long double value */
		_Longlong li;
		_ULonglong uli;
#if !defined(_ADI_COMPILER)
/* Allow for double size switch */
		long
#endif
		     double ld;
		} v;
	void *(*pfn)(void *, const char *, size_t);
	void *arg;
	char *s;
	int n0, nz0, n1, nz1, n2, nz2;
	int argno, prec, nchar, width;
	unsigned short flags;
	char qual;
	char secure;	/* added for _HAS_LIB_EXT1 */
	char sep;	/* added for _HAS_FIXED_POINT %v */
	} _Pft;

typedef struct
	{	/* scan formatting information */
	int (*pfn)(void *, int, int);
	void *arg;
	va_list ap;
	const char *s;
	int nchar, nget, width;
	size_t prec;
	char noconv, qual, stored;
	char secure;	/* added for _HAS_LIB_EXT1 */
	char sep;	/* added for _HAS_FIXED_POINT %v */
	} _Sft;

 #if _HAS_C9X

 #else /* _IS_C9X */

 #if !defined(_SYS_INT_TYPES_H) && !defined(_INTMAXT)
  #define _INTMAXT
typedef _Longlong intmax_t;
typedef _ULonglong uintmax_t;
 #endif /* !defined(_SYS_INT_TYPES_H) etc. */

 #endif /* _IS_C9X */

		/* declarations */
_C_LIB_DECL
#if defined(_ADI_COMPILER)
int _Fflushall(void);

#pragma inline
static void
_Closreg(void)
	{
	__sys_atexit_funcs[_SYS_ATEXIT_STDIO_SLOT] = (atexit_func_t)_Fflushall;
	}
#else
void _Closreg(void);
FILE *_Fofind(void);
#endif  /* _ADI_COMPILER */
void _Fofree(FILE *);
FILE *_Foprep(const _Sysch_t *, const _Sysch_t *,
	FILE *, _FD_TYPE, int);
_FD_TYPE _Fopen(const _Sysch_t *, unsigned int, int);
int _Frprep(FILE *);
int _Ftmpnam(char *, int);
int _Fwprep(FILE *);
void _Genld(_Pft *, char, char *, short, short);
#if defined(_ADI_COMPILER) && !defined(__DOUBLES_ARE_FLOATS__)
#pragma linkage_name __Getfld64
#endif
int _Getfld(_Sft *);
#if defined(_ADI_COMPILER) && !defined(__DOUBLES_ARE_FLOATS__)
#pragma linkage_name __Getfloat64
#endif
int _Getfloat(_Sft *, void *);
int _Getint(_Sft *, void *);
int _Getstr(_Sft *, int);
#if (_ADI_COMPILER)
int _Getfx(_Sft *_px);
#endif  /* _ADI_COMPILER */
void _Ldtob(_Pft *, char);
#if defined (_ADI_COMPILER)
#if defined(__DOUBLES_ARE_FLOATS__)
#pragma linkage_name __Fdtob
#else
#pragma linkage_name __Ldtob
#endif
void _Dtob(_Pft *, char);
#endif /* _ADI_COMPILER */
void _Litob(_Pft *, char);
#if defined(_ADI_COMPILER) && !defined(__DOUBLES_ARE_FLOATS__)
#pragma linkage_name __Printf64
#endif
int _Printf(void *(*)(void *, const char *, size_t),
	void *, const char *, va_list, int);
#if defined(_ADI_COMPILER) && !defined(__DOUBLES_ARE_FLOATS__)
#pragma linkage_name __Putfld64
#endif
int _Putfld(_Pft *, va_list *, char, char *);
int _Putstr(_Pft *, const wchar_t *);
int _Puttxt(_Pft *, const char *);
#if defined(_ADI_COMPILER) && !defined(__DOUBLES_ARE_FLOATS__)
#pragma linkage_name __Scanf64
#endif
int _Scanf(int (*)(void *, int, int),
	void *, const char *, va_list, int);
void _Vacopy(va_list *, va_list);

long _Fgpos(FILE *_str, fpos_t *_ptr);
int _Fspos(FILE *_str, const fpos_t *_ptr, long _loff, int _way);

_END_C_LIB_DECL
_C_STD_END

#if defined (_ADI_COMPILER) && defined (_MISRA_RULES)
#pragma diag(pop)
#endif /* _ADI_COMPILER && _MISRA_RULES */

#endif /* _XSTDIO */

/*
 * Copyright (c) 1992-2008 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
V5.03:1134 */
