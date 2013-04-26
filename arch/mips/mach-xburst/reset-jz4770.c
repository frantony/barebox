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
 */

/**
 * @file
 * @brief Resetting an JZ4770-based board
 */

#include <common.h>
#include <io.h>
#include <mach/jz4770_regs.h>
#include <linux/bitops.h>

#define JZ_EXTAL	24000000

#define WDT_TDR		(WDT_BASE + 0x00)
#define WDT_TCER	(WDT_BASE + 0x04)
#define WDT_TCNT	(WDT_BASE + 0x08)
#define WDT_TCSR	(WDT_BASE + 0x0c)

#define WDT_TCSR_PRESCALE_BIT	3
#define WDT_TCSR_PRESCALE_MASK	(0x7 << WDT_TCSR_PRESCALE_BIT)
 #define WDT_TCSR_PRESCALE1	(0x0 << WDT_TCSR_PRESCALE_BIT)
 #define WDT_TCSR_PRESCALE4	(0x1 << WDT_TCSR_PRESCALE_BIT)
 #define WDT_TCSR_PRESCALE16	(0x2 << WDT_TCSR_PRESCALE_BIT)
 #define WDT_TCSR_PRESCALE64	(0x3 << WDT_TCSR_PRESCALE_BIT)
 #define WDT_TCSR_PRESCALE256	(0x4 << WDT_TCSR_PRESCALE_BIT)
 #define WDT_TCSR_PRESCALE1024	(0x5 << WDT_TCSR_PRESCALE_BIT)
#define WDT_TCSR_EXT_EN		BIT(2)
#define WDT_TCSR_RTC_EN		BIT(1)
#define WDT_TCSR_PCK_EN		BIT(0)

#define WDT_TCER_TCEN		BIT(0)

void __noreturn reset_cpu(ulong addr)
{
	writew(WDT_TCSR_PRESCALE4 | WDT_TCSR_EXT_EN, (u16 *)WDT_TCSR);
	writew(0, (u16 *)WDT_TCNT);

	/* reset after several ms */
	writew(JZ_EXTAL / 1000, (u16 *)WDT_TDR);
	/* enable wdt clock */
	writel(TSCR_WDT, (u32 *)TCU_TSCR);
	/* start wdt */
	writeb(WDT_TCER_TCEN, (u8 *)WDT_TCER);

	unreachable();
}
EXPORT_SYMBOL(reset_cpu);
