/*
 * Copyright (C) 2014 Antony Pavlov <antonynpavlov@gmail.com>
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
#include <ns16550.h>

#include <mach/serial.h>

static int davinci_init(void)
{
	arm_add_mem_device("ram0", 0x82000000, SZ_16M);

	return 0;
}
mem_initcall(davinci_init);

static struct NS16550_plat serial_plat = {
	.clock = 24000000,
	.shift = 2,
};

static int console_init(void)
{
	barebox_set_model("virt2real");
	barebox_set_hostname("virt2real");

	add_ns16550_device(DEVICE_ID_DYNAMIC, DAVINCI_UART0_BASE,
		8 << serial_plat.shift, IORESOURCE_MEM_8BIT, &serial_plat);

	return 0;
}
console_initcall(console_init);
