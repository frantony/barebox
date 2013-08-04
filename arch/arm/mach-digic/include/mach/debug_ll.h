/*
 * Copyright (C) 2013 Antony Pavlov <antonynpavlov@gmail.com>
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

#ifndef __MACH_DEBUG_LL_H__
#define __MACH_DEBUG_LL_H__

#include <io.h>

/* _sleep(0x400000); -> 3 seconds on Canon EOS 600D */
static inline void _sleep(int delay)
{
	int i;

	for (i = 0; i < delay; i++) {
		asm ("nop\n");
		asm ("nop\n");
	}
}

/* Serial interface registers */
#define UART_BASE       0xC0800000
#define UART_TX         (UART_BASE + 0x0)
#define UART_RX         (UART_BASE + 0x4)
#define UART_ST         (UART_BASE + 0x14)

static inline void PUTC_LL(char ch)
{
	/* FIXME! need check UART status first */

	writeb(ch, UART_TX);
	_sleep(0x1000);
}

#endif
