/*
 * ext_watchdog.c - driver for aio external watchdog
 *
 * Copyright (c) 2014 CVTE.SeeCare
 *
 * Licensed under the GPL-2 or later.
 */

#include <common.h>
#include <watchdog.h>
#include <asm/gpio.h>

static unsigned ext_wd_pin;
static bool IsInit = false;

//triggered by a positive or negative transition at WDI.
void bf_ext_wd_reset(void)
{
    static bool state = true;
    
    if (false == IsInit) //init pin first
    {
        ext_wd_pin = CFG_EXT_WATCHDOG_WDI;
        IsInit = false;
        
        if (gpio_request(ext_wd_pin, "Ext_WD"))
        {
            printf("could not request gpio %u\n", ext_wd_pin);
            return ;
        }
        gpio_direction_output(ext_wd_pin, 0);
        IsInit = true;
        //gpio_free(ext_wd_pin);
        return;
    }
    
    if (state)
    {
        gpio_set_value(ext_wd_pin, 1);
    }
    else
    {
        gpio_set_value(ext_wd_pin, 0);
    }
    state = !state;
}

void bf_ext_watchdog_reset(void)
{
    WATCHDOG_RESET();
}

void bf_ext_watchdog_init(void)
{
    WATCHDOG_RESET();
}

