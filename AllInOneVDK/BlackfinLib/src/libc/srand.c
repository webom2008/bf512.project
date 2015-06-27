/************************************************************************
 *
 * srand.c: $Revision: 1.7 $
 *
 * (c) Copyright 2009 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#include "xrand.h"

void
(srand)(unsigned int seed)
{
    _TLV(__randstate).init = 0;
    _TLV(__randstate).seed = seed - 1;
}
