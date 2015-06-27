/*
** Copyright (C) 2004 Analog Devices, Inc. All Rights Reserved.
**
** Wide Character routines are not supported in libio. The
** (lower-performing) full I/O library is available if wide
** characters are required.
*/

/*
** The following symbol is not defined in the library. Any
** attempt to use wide-character routines when using libio
** will cause a linkage error.
*/

.file_attr libName=libio;
.file_attr libGroup=stdio.h;
.file_attr libFunc=_btowc;
.file_attr FuncName=_btowc;
.file_attr libFunc=_wcstod;
.file_attr FuncName=_wcstod;
.file_attr libFunc=_wcstoul;
.file_attr FuncName=_wcstoul;
.file_attr libFunc=_wcstof;
.file_attr FuncName=_wcstof;
.file_attr libFunc=_wcstold;
.file_attr FuncName=_wcstold;
.file_attr libFunc=_wctob;
.file_attr FuncName=_wctob;
.file_attr libFunc=_fgetwc;
.file_attr FuncName=_fgetwc;
.file_attr libFunc=_fgetws;
.file_attr FuncName=_fgetws;
.file_attr libFunc=_fputwc;
.file_attr FuncName=_fputwc;
.file_attr libFunc=_fputws;
.file_attr FuncName=_fputws;
.file_attr libFunc=_fwide;
.file_attr FuncName=_fwide;
.file_attr libFunc=_fwprintf;
.file_attr FuncName=_fwprintf;
.file_attr libFunc=__fwprintf64;
.file_attr FuncName=__fwprintf64;
.file_attr libFunc=_fwscanf;
.file_attr FuncName=_fwscanf;
.file_attr libFunc=__fwscanf64;
.file_attr FuncName=__fwscanf64;
.file_attr libFunc=_getwc;
.file_attr FuncName=_getwc;
.file_attr libFunc=_getwchar;
.file_attr FuncName=_getwchar;
.file_attr libFunc=_putwc;
.file_attr FuncName=_putwc;
.file_attr libFunc=_putwchar;
.file_attr FuncName=_putwchar;
.file_attr libFunc=_swprintf;
.file_attr FuncName=_swprintf;
.file_attr libFunc=__swprintf64;
.file_attr FuncName=__swprintf64;
.file_attr libFunc=_swscanf;
.file_attr FuncName=_swscanf;
.file_attr libFunc=__swscanf64;
.file_attr FuncName=__swscanf64;
.file_attr libFunc=_ungetwc;
.file_attr FuncName=_ungetwc;
.file_attr libFunc=_vfwprintf;
.file_attr FuncName=_vfwprintf;
.file_attr libFunc=__vfwprintf64;
.file_attr FuncName=__vfwprintf64;
.file_attr libFunc=_vswprintf;
.file_attr FuncName=_vswprintf;
.file_attr libFunc=__vswprintf64;
.file_attr FuncName=__vswprintf64;
.file_attr libFunc=_vwprintf;
.file_attr FuncName=_vwprintf;
.file_attr libFunc=__vwprintf64;
.file_attr FuncName=__vwprintf64;
.file_attr libFunc=_wprintf;
.file_attr FuncName=_wprintf;
.file_attr libFunc=__wprintf64;
.file_attr FuncName=__wprintf64;
.file_attr libFunc=_wscanf;
.file_attr FuncName=_wscanf;
.file_attr libFunc=__wscanf64;
.file_attr FuncName=__wscanf64;
.file_attr libFunc=_mbrlen;
.file_attr FuncName=_mbrlen;
.file_attr libFunc=_mbrtowc;
.file_attr FuncName=_mbrtowc;
.file_attr libFunc=_mbsrtowcs;
.file_attr FuncName=_mbsrtowcs;
.file_attr libFunc=_mbsinit;
.file_attr FuncName=_mbsinit;
.file_attr libFunc=_wcrtomb;
.file_attr FuncName=_wcrtomb;
.file_attr libFunc=_wcsrtombs;
.file_attr FuncName=_wcsrtombs;
.file_attr libFunc=_wcstol;
.file_attr FuncName=_wcstol;
.file_attr libFunc=_wcscat;
.file_attr FuncName=_wcscat;
.file_attr libFunc=_wcscmp;
.file_attr FuncName=_wcscmp;
.file_attr libFunc=_wcscoll;
.file_attr FuncName=_wcscoll;
.file_attr libFunc=_wcscpy;
.file_attr FuncName=_wcscpy;
.file_attr libFunc=_wcscspn;
.file_attr FuncName=_wcscspn;
.file_attr libFunc=_wcslen;
.file_attr FuncName=_wcslen;
.file_attr libFunc=_wcsncat;
.file_attr FuncName=_wcsncat;
.file_attr libFunc=_wcncmp;
.file_attr FuncName=_wcncmp;
.file_attr libFunc=_wcsncpy;
.file_attr FuncName=_wcsncpy;
.file_attr libFunc=_wcspn;
.file_attr FuncName=_wcspn;
.file_attr libFunc=_wcsxfrm;
.file_attr FuncName=_wcsxfrm;
.file_attr libFunc=_wmemcmp;
.file_attr FuncName=_wmemcmp;
.file_attr libFunc=_wmemcpy;
.file_attr FuncName=_wmemcpy;
.file_attr libFunc=_wmemmove;
.file_attr FuncName=_wmemmove;
.file_attr libFunc=_wmemset;
.file_attr FuncName=_wmemset;
.file_attr libFunc=_wcsftime;
.file_attr FuncName=_wcsftime;
.file_attr prefersMem=external;
.file_attr prefersMemNum="70";



.extern wide_character_IO_not_supported_in_libio;

.section/DOUBLEANY program;
.align 2;

.MESSAGE/SUPPRESS 1121;  // No point providing end labels here.

_btowc:
_wcstod:
_wcstoul:
_wcstof:
_wcstold:
_wctob:
_fgetwc:
_fgetws:
_fputwc:
_fputws:
_fwide:
_fwprintf:
__fwprintf64:
_fwscanf:
__fwscanf64:
_getwc:
_getwchar:
_putwc:
_putwchar:
_swprintf:
__swprintf64:
_swscanf:
__swscanf64:
_ungetwc:
_vfwprintf:
__vfwprintf64:
_vswprintf:
__vswprintf64:
_vwprintf:
__vwprintf64:
_wprintf:
__wprintf64:
_wscanf:
__wscanf64:
_mbrlen:
_mbrtowc:
_mbsrtowcs:
_mbsinit:
_wcrtomb:
_wcsrtombs:
_wcstol:
_wcscat:
_wcscmp:
_wcscoll:
_wcscpy:
_wcscspn:
_wcslen:
_wcsncat:
_wcncmp:
_wcsncpy:
_wcspn:
_wcsxfrm:
_wmemcmp:
_wmemcpy:
_wmemmove:
_wmemset:
_wcsftime:
	CALL.X wide_character_IO_not_supported_in_libio;

.global _fgetwc;
.global _fgetws;
.global _fputwc;
.global _fputws;
.global _fwide;
.global _fwprintf;
.global __fwprintf64;
.global _fwscanf;
.global __fwscanf64;
.global _getwc;
.global _getwchar;
.global _putwc;
.global _putwchar;
.global _swprintf;
.global __swprintf64;
.global _swscanf;
.global __swscanf64;
.global _ungetwc;
.global _vfwprintf;
.global __vfwprintf64;
.global _vswprintf;
.global __vswprintf64;
.global _vwprintf;
.global __vwprintf64;
.global _wprintf;
.global __wprintf64;
.global _wscanf;
.global __wscanf64;
.global _mbrlen;
.global _mbrtowc;
.global _mbsrtowcs;
.global _mbsinit;
.global _wcrtomb;
.global _wcsrtombs;
.global _wcstol;
.global _wcscat;
.global _wcscmp;
.global _wcscoll;
.global _wcscpy;
.global _wcscspn;
.global _wcslen;
.global _wcsncat;
.global _wcncmp;
.global _wcsncpy;
.global _wcspn;
.global _wcsxfrm;
.global _wmemcmp;
.global _wmemcpy;
.global _wmemmove;
.global _wmemset;
.global _wcsftime;
.global _btowc;
.global _wcstod;
.global _wcstoul;
.global _wcstof;
.global _wcstold;
.global _wctob;
