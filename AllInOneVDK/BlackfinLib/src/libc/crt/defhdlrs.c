/* Copyright (C) 2003 Analog Devices Inc., All Rights Reserved.
*/
#pragma file_attr(  "libName=libevent")
#pragma file_attr(  "libFunc=_install_default_handlers")
#pragma file_attr(  "libFunc=__install_default_handlers")
#pragma file_attr(  "prefersMem=any")
#pragma file_attr(  "prefersMemNum=50")

/*
** By default, we do not install any event handlers beyond:
** - IVG15, to switch from highest supervisor mode (reset)
**   to lowest;
** - An exception handler that deals only with CPLB misses,
**   if CPLB handling is enabled.
**
** This function is invoked to install other handlers, say
** for interrupts, NMI, etc. But the default configuration
** does not do so. If you wish to install others by default,
** you can replace this routine with one that does so.
** Consequently, you may also wish to modify the default
** IMASK that is passed in, to allow your event to occur.
*/

#pragma file_attr("requiredForROMBoot")

int _install_default_handlers(int imask)
{
	return imask;
}
