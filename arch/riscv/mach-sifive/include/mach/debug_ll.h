/*
 * Copyright (C) 2016 Antony Pavlov <antonynpavlov@gmail.com>
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

#ifndef __MACH_SIFIVE_DEBUG_LL__
#define __MACH_SIFIVE_DEBUG_LL__

/** @file
 *  This File contains declaration for early output support
 */

#include <linux/kconfig.h>
#include <asm/io.h>

#define UART_BASE 0x40002000
#define R_DATA          (0 << 2)
#define R_RXCNT         (2 << 2)

static inline void PUTC_LL(int ch)
{
	if (IS_ENABLED(CONFIG_DEBUG_LL))
		__raw_writeb(ch, (u8 *)UART_BASE + R_DATA);
}

#endif /* __MACH_SIFIVE_DEBUG_LL__ */
