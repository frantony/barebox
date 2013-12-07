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
#include <io.h>

void __noreturn reset_cpu(ulong addr)
{
	(*(volatile unsigned *)0x1806001C) = 0x01000000;
	/*
	 * Used to command a full chip reset. This is the software equivalent of
	 * pulling the reset pin. The system will reboot with PLL disabled. Always
	 * zero when read.
	 */
	while (1);
	/*NOTREACHED*/
}
EXPORT_SYMBOL(reset_cpu);
