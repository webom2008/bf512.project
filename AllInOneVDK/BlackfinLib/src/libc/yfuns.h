#pragma once
#ifndef __NO_BUILTIN
#pragma system_header /* yfuns.h */
#endif
/************************************************************************
 *
 * yfuns.h
 *
 * (c) Copyright 2002-2008 Analog Devices, Inc.  All rights reserved.
 *
 * Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 *
 ************************************************************************/

/* yfuns.h functions header */
#ifndef _YFUNS
#define _YFUNS

#if defined(_ADI_COMPILER)
#include <stddef.h>
#endif /* defined(_ADI_COMPILER) */

#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_5_2:"ADI header will re-use identifiers")
#pragma diag(suppress:misra_rule_5_3:"ADI header will re-use identifiers")
#pragma diag(suppress:misra_rule_5_4:"ADI header will re-use identifiers")
#pragma diag(suppress:misra_rule_5_5:"ADI header will re-use identifiers")
#pragma diag(suppress:misra_rule_5_6:"ADI header will re-use identifiers")
#pragma diag(suppress:misra_rule_5_7:"ADI header will re-use identifiers")
#pragma diag(suppress:misra_rule_6_3:"ADI header allows use of basic types")
#pragma diag(suppress:misra_rule_8_8:"_Exit defined if stdio.h included")
#pragma diag(suppress:misra_rule_19_4:"ADI header allows any substitution")
#pragma diag(suppress:misra_rule_19_7:"ADI header allows function macros")
#endif /* _MISRA_RULES */

_C_STD_BEGIN

 #if _USE_EXISTING_SYSTEM_NAMES
  #if defined(__BORLANDC__)
   #define _Environ   _environ

  #else /* defined(__BORLANDC__) */
   #define _Environ   environ
  #endif /* defined(__BORLANDC__) */

 #if _HAS_C9X

 #else /* _IS_C9X */
 #define _Exit        _exit
 #endif /* _IS_C9X */

  #define _Close      close
  #define _Lseek      lseek
  #define _Read               read
  #define _Write      write
 #endif /* _USE_EXISTING_SYSTEM_NAMES */

		/* process control */
#define _Envp	(*_Environ)

		/* stdio functions */
#define _Fclose(str)  _Close(_FD_NO(str))
#define _Fread(str, buf, cnt) _Read(_FD_NO(str), buf, cnt)
#define _Fwrite(str, buf, cnt)        _Write(_FD_NO(str), buf, cnt)

		/* interface declarations */
_EXTERN_C
extern const char **_Environ;

#ifdef __ADSPBLACKFIN__
  void _Exit(void);
#else
  void _Exit(int);
#endif

int _Close(int n);
#ifdef  __ADSPTS__
  int _Read(int, char *, int);
  int _Write(int, char *, int);
#else
  int _Read(int _f, unsigned char *_p, int _n);
  int _Write(int _f, const unsigned char *_pc, int _n);
#endif /* __ADSPTS__ */

_END_EXTERN_C
_C_STD_END

#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */

#endif /* _YFUNS */

/*
 * Copyright (c) 1992-2008 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
V5.03:1134 */
