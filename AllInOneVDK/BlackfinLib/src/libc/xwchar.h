/* xwchar.h internal header */
#if defined (_ADI_COMPILER)
/************************************************************************
 *
 * xwchar.h
 *
 * (c) Copyright 2001-2009 Analog Devices, Inc.  All rights reserved.
 *
 * Copyright (c) 1992-2008 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 *
 ************************************************************************/

#pragma once
#ifndef __NO_BUILTIN
#pragma system_header /* xwchar.h */
#endif
#endif /* _ADI_COMPILER */

#ifndef _XWCHAR
#define _XWCHAR
#include <wchar.h>
#include <wctype.h>
#if defined (_ADI_COMPILER)
#include "xsync.h"
#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_5_6)
#pragma diag(suppress:misra_rule_5_7)
#pragma diag(suppress:misra_rule_6_3)
#pragma diag(suppress:misra_rule_8_8:"_WStold and _WStoul defined in wchar.h")
#pragma diag(suppress:misra_rule_18_1)
#pragma diag(suppress:misra_rule_19_4)
#endif /* _MISRA_RULES */
#else
#include <xstate.h>
#endif /* _ADI_COMPILER */
_C_STD_BEGIN

		/* DECLARATIONS */
_C_LIB_DECL
int _Mbtowc(wchar_t *, const char *, size_t, mbstate_t *);
size_t _Wcsftime(wchar_t *, size_t, const char *, size_t, const struct tm *);
int _Wctomb(char *, wchar_t, mbstate_t *);
long double _WStold(const wchar_t *, wchar_t **, long);
_Longlong _WStoll(const wchar_t *, wchar_t **, int);
unsigned long _WStoul(const wchar_t *, wchar_t **, int);
_ULonglong _WStoull(const wchar_t *, wchar_t **, int);

#if defined (_ADI_COMPILER)
typedef struct {
      mbstate_t mbtowc;
      mbstate_t mbrlen;
      mbstate_t mbrtowc;
      mbstate_t mbsrtowcs;
      mbstate_t wctomb;
      mbstate_t wcrtomb;
      mbstate_t wcsrtombs;
} __mbstates_t;

_TLV_DECL(__mbstates_t, __mbstates);

#else
_Statab *_Getpmbstate(void);
_Statab *_Getpwcstate(void);
_Statab *_Getpcostate(void);
_Statab *_Getpwcostate(void);
#endif  /* _ADI_COMPILER */
_END_C_LIB_DECL
_C_STD_END

#if defined (_ADI_COMPILER) && defined (_MISRA_RULES)
#pragma diag(pop)
#endif /* _ADI_COMPILER && _MISRA_RULES */

#endif /* _XWCHAR */

/*
 * Copyright (c) 1992-2008 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
V5.03:1134 */
