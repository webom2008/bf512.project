/*
 * Blackfin Boot Mode Button
 *
 * Copyright (c) 2014 CVTE SeeCare Inc.
 *
 * Licensed under the GPL-2 or later.
 */

#include <common.h>
#include <config.h>
#include <bfin_button.h>
#include <asm/gpio.h>

#ifdef CONFIG_BOOT_BUTTON
/***********************************************************
*
* FUNCTIONS: Blackfin GPIO Driver For Boot SW
*
* INPUTS/OUTPUTS:   int flags, reserved
* 
* RETRUN:   0--LOW 
*           1--HIGH
*           -1--failed
*
* DESCRIPTION: Blackfin GPIO Driver API
*
* CAUTION:
*************************************************************
* MODIFICATION HISTORY :
**************************************************************/
int button_boot_test(int flags)
{
	int result = 0;
	unsigned button = CFG_BOOT_BUTTON_PIN;
	unsigned short value = 0;

	if (gpio_request(button, "boot"))
    {
		printf("could not request gpio %u\n", button);
		return -1;
	}
	gpio_direction_input(button);

	value = gpio_get_value(button);
	//udelay(10000);
	if (value != 0)
	{
		result = 1;
    }
	else
    {
		result = 0;
	}

	gpio_free(button);

	return result;
}
#endif /* CONFIG_BOOT_BUTTON */

