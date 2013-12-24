/*
 * Copyright (C) 2013 Du Huanpeng <u74147@gmail.com>
 *
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <init.h>
#include <sizes.h>
#include <asm/memory.h>
#include <spi/spi.h>
#include <spi/flash.h>

#include <mach/ar71xx_regs.h>

static int model_hostname_init(void)
{
	barebox_set_model("TP Link WR720N");
	barebox_set_hostname("wr720n");

	return 0;
}
postcore_initcall(model_hostname_init);

static int mem_init(void)
{
	mips_add_ram0(SZ_32M);

	return 0;
}
mem_initcall(mem_init);

static struct flash_platform_data spi_boot_flash_data = {
	.name		= "spi0",
	.type		= "s25sl032p",
};

static struct spi_board_info spi_devs[] __initdata = {
	{
		/* boot: Spansion S25FL032PIF SPI flash */
		.name		= "m25p80",
		.max_speed_hz	= 104 * 1000 * 1000,
		.bus_num	= 0,
		.chip_select	= 0,
		.mode		= SPI_MODE_3,
		.platform_data	= &spi_boot_flash_data,
	},
};

static int devices_init(void)
{
	spi_register_board_info(spi_devs, ARRAY_SIZE(spi_devs));

	add_generic_device("ar933x_spi", 0, NULL,
		KSEG1ADDR(AR71XX_SPI_BASE), AR71XX_SPI_SIZE,
		IORESOURCE_MEM, NULL);

	return 0;
}
device_initcall(devices_init);
