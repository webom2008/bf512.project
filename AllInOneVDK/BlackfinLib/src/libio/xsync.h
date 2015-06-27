/****************************************************************************
 *
 * xsync.h : $Revision: 1.4.14.5 $
 *
 * Library-internal synchronisation facilities.
 *
 * (c) Copyright 2009-2010 Analog Devices, Inc.  All rights reserved.
 *
 ****************************************************************************/

#ifndef _XSYNC_H
#define _XSYNC_H

#include <stdlib.h>
#include <xsyslock.h>

#include "sys_alloc.h"

#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_8_1:"Inline functions don't need prototypes")
#pragma diag(suppress:misra_rule_8_5:"Allow inline function definitions")
#pragma diag(suppress:misra_rule_19_10:"Allow macro params with parentheses")
#pragma diag(suppress:misra_rule_19_12:"Allow multiple ## operators")
#pragma diag(suppress:misra_rule_19_13:"Allow any ## operators")
#endif /* _MISRA_RULES */

_EXTERN_C

#if defined(_ADI_THREADS)

/*
 * Threadsafe build: map to adi_rtl_sync functions
 */

/* The globals lock */

#define _LOCK_GLOBALS adi_rtl_lock_globals
#define _UNLOCK_GLOBALS adi_rtl_unlock_globals


/* Mutexes */

typedef adi_rtl_mutex MUTEX;

#define _DECL_MUTEX(mutex) extern adi_rtl_mutex (mutex);
#define _DEF_MUTEX(mutex) adi_rtl_mutex mutex = ADI_RTL_INVALID_MUTEX;

#define _INIT_MUTEX adi_rtl_init_mutex
#define _DEINIT_MUTEX adi_rtl_deinit_mutex
#define _ACQUIRE_MUTEX adi_rtl_acquire_mutex
#define _RELEASE_MUTEX adi_rtl_release_mutex
#define _CHECK_AND_ACQUIRE_MUTEX __check_and_acquire_mutex


/* Thread-local variables */

/* Each thread-local variable used by the libraries occupies one word (i.e.
 * one void pointer) in the thread-local storage block provided through
 * adi_rtl_get_tls_ptr(). This enumeration defines the position in the TLS
 * block for each variable.
 * _TLV_NUM is the overall number of TLVs. This must be smaller than
 * ADI_RTL_TLS_LEN in sys/adi_rtl_sync.h.
 */
enum {
    __errno_tlv_index,
    __strtok_tlv_index,
    __rand_tlv_index,
    __asctime_tlv_index,
    __tm_tlv_index,
#if !defined(__ADSP21000__)
    /* No strerror or wchar support on SHARC. */
    __strerror_tlv_index,
    __wchar_tlv_index,
#endif
    __eh_tlv_index,
    _TLV_NUM
};

/* Thread-local variables that are the size of a void pointer (or less) are
 * stored directly in the TLS block. Bigger TLVs are allocated on the heap,
 * with pointers to them stored in their TLS block words.
 * All the variables with indices from __first_heap_tlv_index onwards are
 * of the latter kind. The TLV destructor routine in xtlv.c needs to know
 * this to be able to free the heap-allocated TLVs.
 * On SHARC, rand state is a single word, whereas on Blackfin it's a struct.
 */
#if defined(__ADSP21000__)
enum { __first_heap_tlv_index = __asctime_tlv_index };
#else
enum { __first_heap_tlv_index = __rand_tlv_index };
#endif

/* TLV declaration: declares an access function */
#define _TLV_DECL(type, name) \
  _Pragma("const") type *__get_##name##_ptr(void)

/* TLV definition: defines the access function, which returns a pointer to
 * the variables value. This points to the variables word in the TLS block
 * for word-sized variables, and do its heap block for bigger ones, whereby
 * that block is allocated on demand.
 */
#define _TLV_DEF(index, type, name) \
    type * \
    __get_##name##_ptr(void) \
    { \
        void **slot_p = (void **)adi_rtl_get_tls_ptr() + (index); \
        if ((index) >= __first_heap_tlv_index) \
            return *slot_p ?: (*slot_p = _sys_calloc(sizeof(type), 1)); \
        else if (sizeof(type) <= sizeof(void *)) \
            return (type *)slot_p; \
        else \
            abort(); \
    }

/* _TLV_INLINE can be used on a TLV definition to mark its access function
 * as inline. (This macro is empty for a non-threadsafe build, where TLVs
 * become normal global variables.)
 */
#define _TLV_INLINE __inline

/* TLV access: invokes the access function defined with _TLV_DEF */
#define _TLV(name)  (*__get_##name##_ptr())


/* The exception handling TLV needs special treatment on thread exit,
 * because it contains pointers to further blocks that need to freed.
 */
extern void (*__eh_tlv_dtor)(void *);

#pragma inline
static void
__set_eh_tlv_dtor(void (*dtor)(void *))
{ __eh_tlv_dtor = dtor; }

#else /* defined(_ADI_THREADS) */

/*
 * Non-threadsafe build: consider the program as a single thread.
 */ 

/* The globals lock does nothing. */
#define _LOCK_GLOBALS() (void)0
#define _UNLOCK_GLOBALS() (void)0

/* Mutexes do nothing. */
typedef void MUTEX;
#define _DECL_MUTEX(mutex)
#define _DEF_MUTEX(mutex)
#define _INIT_MUTEX(mutex_p, io) (true)
#define _DEINIT_MUTEX(mutex_p) (void)0
#define _ACQUIRE_MUTEX(mutex_p) (void)0
#define _RELEASE_MUTEX(mutex_p) (void)0
#define _CHECK_AND_ACQUIRE_MUTEX(mutex_p, io) (void)0

/* Thread-local variables turn into global variables. */
#define _TLV_INLINE
#define _TLV_DECL(type, name)  extern type (name)
#define _TLV_DEF(index, type, name)  type (name)
#define _TLV(name)  (name)
#define __set_eh_tlv_dtor(dtor)

#endif /* defined(_ADI_THREADS) */

_END_EXTERN_C

#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */

#endif
