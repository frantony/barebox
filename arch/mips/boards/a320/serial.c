/*
 * Copyright (C) 2012 Antony Pavlov <antonynpavlov@gmail.com>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <common.h>
#include <types.h>
#include <driver.h>
#include <init.h>
#include <ns16550.h>
#include <mach/jz4740_regs.h>
#include <io.h>
#include <asm/common.h>
#include <mach/jz4740_fb.h>

#define JZ4740_UART_SHIFT	2

#define ier		(1 << JZ4740_UART_SHIFT)
#define fcr		(2 << JZ4740_UART_SHIFT)

static void jz4740_serial_reg_write(unsigned int val, unsigned long base,
	unsigned char reg_offset)
{
	switch (reg_offset) {
	case fcr:
		val |= 0x10; /* Enable uart module */
		break;
	case ier:
		val |= (val & 0x4) << 2;
		break;
	default:
		break;
	}

	writeb(val & 0xff, (void *)(base + reg_offset));
}

static struct NS16550_plat serial_plat = {
	.clock = 12000000,
	.shift = JZ4740_UART_SHIFT,
	.reg_write = &jz4740_serial_reg_write,
};

#include <fb.h>

static struct fb_videomode a320_fb_modes[] = {
	{
		.name		= "A320",
		.refresh	= 60,
		.xres		= 320,
		.left_margin	= 21,
		.right_margin	= 38,
		.hsync_len	= 6,
		.yres		= 240,
		.upper_margin	= 4,
		.lower_margin	= 4,
		.vsync_len	= 2,
		.pixclock	= 115913,
		.sync		= 0,
		.vmode		= FB_VMODE_NONINTERLACED,
		.flag		= 0,
	},
};

static struct jz4740_fb_platform_data a320_fb_data = {
	.mode		= a320_fb_modes,
};

static int a320_console_init(void)
{
	/* Register the serial port */
	add_ns16550_device(-1, UART0_BASE, 8 << JZ4740_UART_SHIFT,
			IORESOURCE_MEM_8BIT, &serial_plat);

#ifdef CONFIG_DRIVER_VIDEO_FB_JZ4740
	add_generic_device("jz4740_fb", -1, NULL,
		(resource_size_t)JZ4740_LCD_BASE_ADDR,
		(resource_size_t)JZ4740_LCD_BASE_ADDR + 0x1000 - 1,
		IORESOURCE_MEM, &a320_fb_data);
#endif

	return 0;
}
console_initcall(a320_console_init);

static int a320_devices_init(void)
{
	add_generic_device("jz4740_fb", -1, NULL,
		(resource_size_t)JZ4740_LCD_BASE_ADDR,
		(resource_size_t)JZ4740_LCD_BASE_ADDR + 0x1000 - 1,
		IORESOURCE_MEM, &a320_fb_data);

	return 0;
}
device_initcall(a320_devices_init);
