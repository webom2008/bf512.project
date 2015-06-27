/* xmath.h internal header */
#if defined (_ADI_COMPILER)
/************************************************************************
 *
 * xmath.h
 *
 * (c) Copyright 2001-2009 Analog Devices, Inc.  All rights reserved.
 * (c) Copyright 1992-2008 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * $Revision: 3543 $
 ************************************************************************/

#pragma once
#ifndef __NO_BUILTIN
#pragma system_header /* xmath.h */
#endif
#endif /* _ADI_COMPILER */

#ifndef _XMATH
#define _XMATH
#include <ymath.h>
#include <errno.h>
#include <math.h>
#include <stddef.h>

#if defined (_ADI_COMPILER) && defined (_MISRA_RULES)
#pragma diag(push)
#pragma diag(suppress:misra_rule_5_2:"ADI header will re-use identifiers")
#pragma diag(suppress:misra_rule_5_3:"ADI header will re-use identifiers")
#pragma diag(suppress:misra_rule_5_4:"ADI header will re-use identifiers")
#pragma diag(suppress:misra_rule_5_5:"ADI header will re-use identifiers")
#pragma diag(suppress:misra_rule_5_6:"ADI header will re-use identifiers")
#pragma diag(suppress:misra_rule_5_7:"ADI header will re-use identifiers")
#pragma diag(suppress:misra_rule_6_3:"ADI header allows use of basic types")
#pragma diag(suppress:misra_rule_8_8:"ADI header- prototypes defined twice")
#pragma diag(suppress:misra_rule_19_4:"ADI header allows any substitution")
#pragma diag(suppress:misra_rule_19_7:"ADI header allows function macros")
#endif /* _ADI_COMPILER && _MISRA_RULES */

_C_STD_BEGIN

		/* FLOAT PROPERTIES */
#ifndef _D0
 #define _D0	3	/* little-endian, small long doubles */
 #define _D1	2
 #define _D2	1
 #define _D3	0

 #define _DBIAS	0x3fe
 #define _DOFF	4

 #define _FBIAS	0x7e
 #define _FOFF	7
 #define _FRND	1

 #define _DLONG	0
 #define _LBIAS	0x3fe
 #define _LOFF	4

#elif defined (_ADI_COMPILER) && defined(__ADSPBLACKFIN__)
#if defined(__DOUBLES_ARE_FLOATS__)
 /* _D0 set to 1 */
 #define _D1 0       /* little-endian */
 #define _D2 2       /* point beyond 32-bit data */
 #define _D3 2       /* point beyond 32-bit data */

#else
 /* _D0 set to 3 */
 #define _D1 2
 #define _D2 1
 #define _D3 0

#endif  /* __DOUBLES_ARE_FLOATS__ */
#elif defined (_ADI_COMPILER) && defined(__ADSP21000__)
#if defined(__DOUBLES_ARE_FLOATS__)
 #define _D1 1
 #define _D2 0
 #define _D3 0
#else
 #define _D1 1       /* big-endian */
 #define _D2 2
 #define _D3 xxx     /* should never be used */
#endif

#elif _D0 == 0		/* other params defined in <yvals.h> */
 #define _D1	1	/* big-endian */
 #define _D2	2
 #define _D3	3

#else /* _D0 */
 #define _D1	2	/* little-endian */
 #define _D2	1
 #define _D3	0
#endif /* _D0 */

		/* IEEE 754 double properties */
#if defined(__ADSP21000__)
#define _FFRAC  ((unsigned int)(((unsigned int)(unsigned int)1u << (unsigned int)_FOFF) - 1u))
#define _FMASK  ((unsigned int)(0x7fffu & (unsigned int)(~(unsigned int)_FFRAC)))
#define _FMAX   ((unsigned int)((unsigned short)(1u << (15u - (unsigned int)_FOFF)) - 1u))
#define _FSIGN  ((unsigned int)0x8000)
#define FSIGN(x)        (((unsigned int *)&(x))[_F0] & _FSIGN)
#define FHUGE_EXP       (int)(_FMAX * 900L / 1000)
#define FHUGE_RAD       40.7    /* ~ 2^7 / pi */
#define FSAFE_EXP       ((unsigned int)(_FMAX >> 1))
#else  /* __ADSP21000__ */
#define _FFRAC        ((unsigned short)((unsigned int)((unsigned int)1u << (unsigned int)_FOFF) - 1u))
#define _FMASK        ((unsigned short)(0x7fffu & (unsigned int)(~(unsigned int)_FFRAC)))
#define _FMAX ((unsigned short)((unsigned short)(1u << (15u - (unsigned int)_FOFF)) - 1u))
#define _FSIGN        ((unsigned short)0x8000)
#define FSIGN(x)      (((unsigned short *)&(x))[_F0] & _FSIGN)
#define FHUGE_EXP     (int)(_FMAX * 900L / 1000)
#define FHUGE_RAD     40.7    /* ~ 2^7 / pi */
#define FSAFE_EXP     ((unsigned short)(_FMAX >> 1))
#endif /* __ADSP21000__ */

              /* IEEE 754 double properties */
#if ((defined(__ADSPBLACKFIN__) || defined(__ADSP21000__)) \
      && defined(__DOUBLES_ARE_FLOATS__))
                /* doubles are floats */
#define _DFRAC        _FFRAC
#define _DMASK        _FMASK
#define _DMAX _FMAX
#define _DSIGN        _FSIGN
#define DSIGN(x)      FSIGN(x)
#define HUGE_EXP      FHUGE_EXP
#define HUGE_RAD      FHUGE_RAD
#define SAFE_EXP      FSAFE_EXP
#elif (defined(__ADSP21000__) && !defined(__DOUBLES_ARE_FLOATS__))
                /* doubles are long doubles */
#define _DFRAC  ((unsigned int)(-1))
#define _DMASK  ((unsigned int)0x7fff)
#define _DMAX   ((unsigned int)0x7fff)
#define _DSIGN  ((unsigned int)0x8000)
#define DSIGN(x)        (((unsigned int *)&(x))[_L0] & _LSIGN)
#define HUGE_EXP        (int)(_LMAX * 900L / 1000)
#define HUGE_RAD        2.73e9  /* ~ 2^33 / pi */
#define SAFE_EXP        ((unsigned int)(_LMAX >> 1))
#elif (defined(__ADSPBLACKFIN__) && !defined(__DOUBLES_ARE_FLOATS__))
#define _DFRAC	((unsigned short)((1 << _DOFF) - 1))
#define _DMASK	((unsigned short)(0x7fff & ~_DFRAC))
#define _DMAX	((unsigned short)((1 << (15 - _DOFF)) - 1))
#define _DSIGN	((unsigned short)0x8000)
#define DSIGN(x)	(((unsigned short *)(char *)&(x))[_D0] & _DSIGN)
#define HUGE_EXP	(int)(_DMAX * 900L / 1000)
#define HUGE_RAD	2.73e9	/* ~ 2^33 / pi */
#define SAFE_EXP	((short)(_DMAX >> 1))
#endif

#if defined(_ADI_COMPILER)
  #define _F0 1
  #define _F1 0
#else
 #if _D0 == 0
  #define _F0	0	/* big-endian */
  #define _F1	1

 #else /* _D0 == 0 */
  #define _F0	1	/* little-endian */
  #define _F1	0
 #endif /* _D0 == 0 */
#endif  /* _ADI_COMPILER */

		/* IEEE 754 long double properties */
#if defined(_ADI_COMPILER) && defined(__ADSP21000__)
#define _LFRAC  ((unsigned int)(-1))
#define _LMASK  ((unsigned int)0x7fff)
#define _LMAX   ((unsigned int)0x7fff)
#define _LSIGN  ((unsigned int)0x8000)
#define LSIGN(x)        (((unsigned int *)&(x))[_L0] & _LSIGN)
#define LHUGE_EXP       (int)(_LMAX * 900L / 1000)
#define LHUGE_RAD       2.73e9  /* ~ 2^33 / pi */
#define LSAFE_EXP       ((unsigned int)(_LMAX >> 1))
#else
#define _LFRAC	((unsigned short)(-1))
#define _LMASK	((unsigned short)0x7fff)
#define _LMAX	((unsigned short)0x7fff)
#define _LSIGN	((unsigned short)0x8000)
#define LSIGN(x)	(((unsigned short *)(char *)&(x))[_L0] & _LSIGN)
#define LHUGE_EXP	(int)(_LMAX * 900L / 1000)
#define LHUGE_RAD	2.73e9	/* ~ 2^33 / pi */
#define LSAFE_EXP	((short)(_LMAX >> 1))
#endif  /* _ADI_COMPILER */

#if defined(_ADI_COMPILER) && defined(__DOUBLES_ARE_FLOATS__)
                /* doubles are floats */

#define  _Denorm      _FDenorm
#define  _Eps         _FEps
#define  _Hugeval     _FHugeval
#define  _Inf         _FInf
#define  _Nan         _FNan
#define  _Rteps       _FRteps
#define  _Snan        _FSnan
#define  _Xbig        _FXbig
#define  _Zero        _FZero

#elif defined(_ADI_COMPILER) && !defined(__DOUBLES_ARE_FLOATS__)
                /* doubles are long doubles */

#define  _Denorm      _LDenorm
#define  _Eps         _LEps
#define  _Hugeval     _LHugeval
#define  _Inf         _LInf
#define  _Nan         _LNan
#define  _Rteps       _LRteps
#define  _Snan        _LSnan
#define  _Xbig        _LXbig
#define  _Zero        _LZero

#endif /* _ADI_COMPILER && __DOUBLES_ARE_FLOATS__*/

#if defined(_ADI_COMPILER) && defined(__ADSP21000__)
 #define _L0   0       /* big-endian, 64-bit long doubles */
 #define _L1   1
 #define _L2   xxx
 #define _L3   xxx
 #define _L4   xxx     /* should never be used */
 #define _L5   xxx
 #define _L6   xxx
 #define _L7   xxx

 #elif _D0 == 0
  #define _L0	0	/* big-endian */
  #define _L1	1
  #define _L2	2
  #define _L3	3
  #define _L4	4
  #define _L5	5	/* 128-bit only */
  #define _L6	6
  #define _L7	7

 #elif _DLONG == 0
  #define _L0	3	/* little-endian, 64-bit long doubles */
  #define _L1	2
  #define _L2	1
  #define _L3	0
  #define _L4	xxx	/* should never be used */
  #define _L5	xxx
  #define _L6	xxx
  #define _L7	xxx

 #elif _DLONG == 1
  #define _L0	4	/* little-endian, 80-bit long doubles */
  #define _L1	3
  #define _L2	2
  #define _L3	1
  #define _L4	0
  #define _L5	xxx	/* should never be used */
  #define _L6	xxx
  #define _L7	xxx

 #else /* _DLONG */
  #define _L0	7	/* little-endian, 128-bit long doubles */
  #define _L1	6
  #define _L2	5
  #define _L3	4
  #define _L4	3
  #define _L5	2
  #define _L6	1
  #define _L7	0
 #endif /* _DLONG */

		/* return values for _Stopfx/_Stoflt */
#define FL_ERR	0
#define FL_DEC	1
#define FL_HEX	2
#define FL_INF	3
#define FL_NAN	4
#define FL_NEG	8

_C_LIB_DECL
int _Stopfx(const char **, char **);
int _Stoflt(const char *, const char *, char **,
	long[], int);
int _Stoxflt(const char *, const char *, char **,
	long[], int);
int _WStopfx(const wchar_t **, wchar_t **);
int _WStoflt(const wchar_t *, const wchar_t *, wchar_t **,
	long[], int);
 int _WStoxflt(const wchar_t *, const wchar_t *, wchar_t **,
	long[], int);

		/* double declarations */
double _Atan(double, int);
#if defined(_ADI_COMPILER) && defined(__DOUBLES_ARE_FLOATS__)
#pragma linkage_name __FDint
#else
#pragma linkage_name __LDint
#endif
short _Dint(double *, short);
#if defined(_ADI_COMPILER) && !defined(__DOUBLES_ARE_FLOATS__)
#pragma linkage_name __LDnorm
#endif
short _Dnorm(unsigned short *);
#if defined(_ADI_COMPILER) && defined(__DOUBLES_ARE_FLOATS__)
#pragma linkage_name __FDscale
#else
#pragma linkage_name __LDscale
#endif
short _Dscale(double *, long);
#if defined(_ADI_COMPILER) && defined(__DOUBLES_ARE_FLOATS__)
#pragma linkage_name __FDunscale
#else
#pragma linkage_name __LDunscale
#endif
short _Dunscale(short *, double *);
double _Hypot(double, double, int *);
double _Poly(double, const double *, int);
#if defined(_ADI_COMPILER)
extern const  _Dconst _Eps, _Rteps;
#else
extern /* const */ _Dconst _Eps, _Rteps;
extern /* const */ double _Xbig, _Zero;
#endif  /* _ADI_COMPILER */

double _Xp_getw(double *, int);
double *_Xp_setw(double *, int, double);
double *_Xp_addh(double *, int, double);
double *_Xp_mulh(double *, int, double);
double *_Xp_movx(double *, int, double *);
double *_Xp_addx(double *, int, double *, int);
double *_Xp_subx(double *, int, double *, int);
double *_Xp_ldexpx(double *, int, int);
double *_Xp_mulx(double *, int, double *, int, double *);
double *_Xp_invx(double *, int, double *);
double *_Xp_sqrtx(double *, int, double *);

		/* float declarations */
float _FAtan(float, int);
short _FDint(float *, short);
short _FDnorm(unsigned short *);
short _FDscale(float *, long);
short _FDunscale(short *, float *);
float _FHypot(float, float, int *);
float _FPoly(float, const float *, int);
#if defined (_ADI_COMPILER)
/* Make these explicitly const */
extern const _Dconst _FEps, _FRteps;
extern const float _FXbig, _FZero;
#else
extern /* const */ _Dconst _FEps, _FRteps;
extern /* const */ float _FXbig, _FZero;
#endif

float _FXp_getw(float *, int);
float *_FXp_setw(float *, int, float);
float *_FXp_addh(float *, int, float);
float *_FXp_mulh(float *, int, float);
float *_FXp_movx(float *, int, float *);
float *_FXp_addx(float *, int, float *, int);
float *_FXp_subx(float *, int, float *, int);
float *_FXp_ldexpx(float *, int, int);
float *_FXp_mulx(float *, int, float *, int, float *);
float *_FXp_invx(float *, int, float *);
float *_FXp_sqrtx(float *, int, float *);

		/* long double functions */
long double _LAtan(long double, int);
short _LDint(long double *, short);
#if !(defined(_ADI_COMPILER) && defined(__ADSPBLACKFIN__))
short _LDnorm(unsigned short *);
#endif  /* _ADI_COMPILER && __ADSPBLACKFIN__ */
short _LDscale(long double *, long);
short _LDunscale(short *, long double *);
long double _LHypot(long double, long double, int *);
long double _LPoly(long double, const long double *, int);
#if defined (_ADI_COMPILER)
/* make explcitly const */
extern  const  _Dconst _LEps, _LRteps;
extern  const  long double _LXbig, _LZero;
#else
extern /* const */ _Dconst _LEps, _LRteps;
extern /* const */ long double _LXbig, _LZero;
#endif

long double _LXp_getw(long double *, int);
long double *_LXp_setw(long double *, int, long double);
long double *_LXp_addh(long double *, int, long double);
long double *_LXp_mulh(long double *, int, long double);
long double *_LXp_movx(long double *, int, long double *);
long double *_LXp_addx(long double *, int,
	long double *, int);
long double *_LXp_subx(long double *, int,
	long double *, int);
long double *_LXp_ldexpx(long double *, int, int);
long double *_LXp_mulx(long double *, int, long double *,
	int, long double *);
long double *_LXp_invx(long double *, int, long double *);
long double *_LXp_sqrtx(long double *, int, long double *);
_END_C_LIB_DECL

_C_STD_END

#if defined (_ADI_COMPILER) && defined (_MISRA_RULES)
#pragma diag(pop)
#endif /* _ADI_COMPILER && _MISRA_RULES */


#endif /* _XMATH */

/*
 * Copyright (c) 1992-2008 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
V5.03:1134 */
