/*
 * DaVinci GPIO Platform Related Defines
 *
 * Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __DAVINCI_GPIO_PLATFORM_H
#define __DAVINCI_GPIO_PLATFORM_H

#include <io.h>
#include <asm-generic/gpio.h>

struct davinci_gpio_platform_data {
	u32	ngpio;
	u32	gpio_unbanked;
};


struct davinci_gpio_controller {
	struct gpio_chip	chip;
	/* Serialize access to GPIO registers */
	void __iomem		*regs;
	void __iomem		*set_data;
	void __iomem		*clr_data;
	void __iomem		*in_data;
};

#endif
