/* Copyright (C) 2000-2004 Analog Devices Inc., All Rights Reserved,
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
*/
/* This file (devtab.c) initilizes the Device Driver table. When new 
 * device libraries are added, the address to the Device Entry Structure 
 * defined for that device has to be added to the table.  
 * The zeros signify null pointers
 */
#pragma file_attr(  "libName=libio")
#pragma file_attr(  "libGroup=device.h")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include "device.h"

#ifdef PRIMIO
extern DevEntry     primio_deventry;		/* Godot/_PrimIO interface */
#endif

DevEntry_t DevDrvTable[MAXDEV] = {
#ifdef PRIMIO
  &primio_deventry,
#endif
  0,
};

