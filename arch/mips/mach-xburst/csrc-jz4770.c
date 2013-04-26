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
 * @brief Clocksource based on JZ4770 timer
 */

#include <init.h>
#include <clock.h>
#include <io.h>
#include <mach/jz4770_regs.h>

#define JZ_TIMER_CLOCK 12000000

static uint64_t jz4770_cs_read(void)
{
	/* we use only low 32-bit ot the timer register */
	return ((uint64_t)readl((void *)OST_OSTCNTL));
}

static struct clocksource jz4770_cs = {
	.read	= jz4770_cs_read,
	.mask   = CLOCKSOURCE_MASK(32),
};

static int clocksource_init(void)
{
	clocks_calc_mult_shift(&jz4770_cs.mult, &jz4770_cs.shift,
		JZ_TIMER_CLOCK / 16, NSEC_PER_SEC, 10);
	init_clock(&jz4770_cs);

	writel(OSTCSR_PRESCALE16 | OSTCSR_EXT_EN | OSTCSR_CNT_MD,
		(void *)OST_OSTCSR);
	writel(0, (void *)OST_OSTCNTL);
	writel(0, (void *)OST_OSTCNTH);
	writel(0xffffffff, (void *)OST_OSTDR);

	/* enable timer clock */
	writel(TSCR_OST, (void *)TCU_TSCR);
	/* start counting up */
	writel(TESR_OST, (void *)TCU_TESR);

	return 0;
}
core_initcall(clocksource_init);
