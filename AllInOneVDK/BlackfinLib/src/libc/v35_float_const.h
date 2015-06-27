/************************************************************************
 *
 * v35_float_const.h : $Revision: 4 $
 *
 * (c) Copyright 2005 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

/*
 * Description :   Wrapper function, mapping entry point names used
 *                 prior to VisualDSP++4.0 to their functional equivalent
 *                 in VisualDSP++4.0. Include file based on float.h.
 */

#ifndef _V35FLOAT
#define _V35FLOAT

/* Originally defined in */
#include <float.h>


                /* TYPE DEFINITIONS */
typedef struct
        {       /* parameters for a floating-point type */
        int _Ddig, _Dmdig, _Dmax10e, _Dmaxe, _Dmin10e, _Dmine;
        union
                {       /* union of short array and all floats */
                float           _Float;          
                unsigned short  _Us[8];
                double          _Double;
                long double     _Long_double;
                } _Deps, _Dmax, _Dmin;
        } _Dvals;

/*
 * Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 * V3.10:1134
 * $Revision: 4 $
 */
#endif /* _V35FLOAT */
