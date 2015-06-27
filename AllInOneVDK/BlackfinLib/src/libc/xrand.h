/****************************************************************************
 *
 * xrand.h : $Revision: 1.5 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 ****************************************************************************/

#ifndef _XRAND_H
#define _XRAND_H

#include "xsync.h"

#define TSIZ 32  /* must be power of two */

typedef struct {
    unsigned long seed;
    unsigned long idx;
    unsigned long rv[TSIZ];
    char init;
} __randstate_t;

_TLV_DECL(__randstate_t, __randstate);

#endif
