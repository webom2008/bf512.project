/* xwstdio.h internal header */
#if defined (_ADI_COMPILER)
/************************************************************************
 *
 * xwstdio.h
 *
 * (c) Copyright 2001-2009 Analog Devices, Inc.  All rights reserved.
 *
 * Copyright (c) 1992-2008 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 *
 ************************************************************************/

#pragma once
#ifndef __NO_BUILTIN
#pragma system_header /* xwstdio.h */
#endif
#endif /* _ADI_COMPILER */

#ifndef _XWSTDIO
#define _XWSTDIO
#include <xstdio.h>
#include <xwchar.h>

#if defined (_ADI_COMPILER) && defined (_MISRA_RULES)
#pragma diag(push)
#pragma diag(suppress:misra_rule_5_1:"Disable rule 5.1 which bars use of long identifiers (>31 chars). This header has many such long macros but they have been available for a long time so cannot be modified to be compliant.")
#pragma diag(suppress:misra_rule_5_2:"ADI header will re-use identifiers")
#pragma diag(suppress:misra_rule_5_3:"ADI header will re-use identifiers")
#pragma diag(suppress:misra_rule_5_4:"ADI header will re-use identifiers")
#pragma diag(suppress:misra_rule_5_5:"ADI header will re-use identifiers")
#pragma diag(suppress:misra_rule_5_6:"ADI header will re-use identifiers")
#pragma diag(suppress:misra_rule_5_7:"ADI header will re-use identifiers")
#pragma diag(suppress:misra_rule_6_3:"ADI header allows use of basic types")
#pragma diag(suppress:misra_rule_12_12:"ADI header uses float in union")
#pragma diag(suppress:misra_rule_18_4:"ADI header allows unions")
#pragma diag(suppress:misra_rule_19_4:"ADI header allows any substitution")
#pragma diag(suppress:misra_rule_19_7:"ADI header allows function macros")
#endif /* _ADI_COMPILER && _MISRA_RULES */

_C_STD_BEGIN

#if defined (_ADI_COMPILER)
/* backup definition of va_copy */
# ifndef va_copy
 #define va_copy(dest, src)   ((dest) = (src))
# endif
#endif /* _ADI_COMPILER */

		/* macros for _WScanf and friends */
#define WGET(px)	\
	(++(px)->nchar, (*(px)->pfn)((px)->arg, 0, 1))
#define WGETN(px)	(wint_t)(0 <= --(px)->nget ? WGET(px) \
	: (++(px)->nchar, WEOF))
#define WUNGET(px, ch)	\
	(--(px)->nchar, (*(px)->pfn)((px)->arg, ch, 0))
#define WUNGETN(px, ch)	do if ((int)(ch) != WEOF) WUNGET(px, ch); \
	else --(px)->nchar; while (0)

		/* type definitions */
typedef struct
	{	/* print formatting information */
	union
		{	/* long or long double value */
		_Longlong li;
		_ULonglong uli;
#if !defined (_ADI_COMPILER)
/* Use the double type to allow for switchable size */
		long
#endif
		     double ld;
		} v;
	void *(*pfn)(void *, const wchar_t *, size_t);
	void *arg;
	wchar_t *s;
	int n0, nz0, n1, nz1, n2, nz2;
	int argno, prec, nchar, width;
	unsigned short flags;
	wchar_t qual;
	char secure;	/* added for _HAS_LIB_EXT1 */
	wchar_t sep;	/* added for _HAS_FIXED_POINT %v */
	} _WPft;

typedef struct
	{	/* scan formatting information */
	wint_t (*pfn)(void *, wint_t, int);
	void *arg;
	va_list ap;
	const wchar_t *s;
	int nchar, nget, width;
	size_t prec;
	wchar_t qual;
	char noconv, stored;
	char secure;	/* added for _HAS_LIB_EXT1 */
	wchar_t sep;	/* added for _HAS_FIXED_POINT %v */
	} _WSft;

		/* declarations */
_C_LIB_DECL
int _WFrprep(FILE *);
int _WFwprep(FILE *);
void _WGenld(_WPft *, wchar_t, wchar_t *, short, short);
#if defined (_ADI_COMPILER) && !defined(__DOUBLES_ARE_FLOATS__)
#pragma linkage_name __WGetfld64
#endif
int _WGetfld(_WSft *);
#if defined (_ADI_COMPILER) && !defined(__DOUBLES_ARE_FLOATS__)
#pragma linkage_name __WGetfloat64
#endif
int _WGetfloat(_WSft *, void *);
int _WGetint(_WSft *, void *);
int _WGetstr(_WSft *, int);
void _WLdtob(_WPft *, wchar_t);
#if defined (_ADI_COMPILER)
#if defined(__DOUBLES_ARE_FLOATS__)
#pragma linkage_name __WFtob
#else
#pragma linkage_name __WLdtob
#endif
void _WDtob(_WPft *_px, wchar_t _code);
#endif /* _ADI_COMPILER */
void _WLitob(_WPft *, wchar_t);
#if defined (_ADI_COMPILER) && !defined(__DOUBLES_ARE_FLOATS__)
#pragma linkage_name __WPrintf64
#endif
int _WPrintf(void *(*)(void *, const wchar_t *, size_t),
	void *, const wchar_t *, va_list, int);
int _WPutstr(_WPft *, const char *);
#if defined (_ADI_COMPILER) && !defined(__DOUBLES_ARE_FLOATS__)
#pragma linkage_name __WPutfld64
#endif
int _WPutfld(_WPft *, va_list *, wchar_t, wchar_t *);
int _WPuttxt(_WPft *, const wchar_t *);
#if defined (_ADI_COMPILER) && !defined(__DOUBLES_ARE_FLOATS__)
#pragma linkage_name __WScanf64
#endif
int _WScanf(wint_t (*)(void *, wint_t, int),
	void *, const wchar_t *, va_list, int);
_END_C_LIB_DECL
_C_STD_END

#if defined (_ADI_COMPILER) && defined (_MISRA_RULES)
#pragma diag(pop)
#endif /* _ADI_COMPILER && _MISRA_RULES */

#endif /* _XWSTDIO */

/*
 * Copyright (c) 1992-2008 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
V5.03:1134 */
