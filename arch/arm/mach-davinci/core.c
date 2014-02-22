/*
 * Copyright (C) 2009 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * GPLv2 only
 */

#include <common.h>
#include <init.h>
#include <sizes.h>
#include <asm/memory.h>
#include <ns16550.h>

//#include <mach/devices.h>

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

	add_ns16550_device(DEVICE_ID_DYNAMIC, 0x01c20000,
		8 << serial_plat.shift, IORESOURCE_MEM_8BIT, &serial_plat);

	return 0;
}
console_initcall(console_init);
