/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2016 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
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

#ifndef __ASSEMBLY__
#include <asm/io.h>
#endif

#define UART_BASE 0x10010000
#define R_DATA          (0 << 2)

#ifndef __ASSEMBLY__
static inline void PUTC_LL(int ch)
{
	if (IS_ENABLED(CONFIG_DEBUG_LL))
		__raw_writeb(ch, (u8 *)UART_BASE + R_DATA);
}
#endif
#endif /* __MACH_SIFIVE_DEBUG_LL__ */
