/*****************************************************************************
Copyright (c), 2002-2009 - Analog Devices Inc. All Rights Reserved.
This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
and its licensors.
******************************************************************************

$Revision: 2095 $
$Date: 2010-02-18 17:08:05 -0500 (Thu, 18 Feb 2010) $

Title: AD7879-1 Touchscreen Controller Driver

Description: This is the primary source file for the AD7879-1 Touchscreen
             Controller Driver. The driver supports Device access commands
             to access AD7879 registers.

             Access to the AD7879-1 registers is over the TWI port using the
             Device Access Service.

Note:        The main implementation is in the driver source for the plain
             AD7879.  This file defines a preprocessor identifier for the 
             AD7879-1 and then includes the main source file.
*****************************************************************************/

#define ADI_IMPLEMENT_AD7879_1

#include "adi_ad7879.c"

