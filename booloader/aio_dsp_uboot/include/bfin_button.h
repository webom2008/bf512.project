/*
 * Blackfin Boot Mode Button
 *
 * Copyright (c) 2014 CVTE SeeCare Inc.
 *
 * Licensed under the GPL-2 or later.
 */


#ifndef _BFIN_BUTTON_H_
#define	_BFIN_BUTTON_H_

#ifdef CONFIG_BOOT_BUTTON

extern int	button_boot_test(int flags);

#endif  /* CONFIG_BOOT_BUTTON	*/

#endif	/* _BFIN_BUTTON_H_	*/
