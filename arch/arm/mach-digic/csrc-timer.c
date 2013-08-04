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

#include <init.h>
#include <clock.h>
#include <io.h>

#include <mach/digic4.h>

#define DIGIC_TIMER_CLOCK 1000000

#define DIGIC_TIMER_CONTROL 0x00
#define DIGIC_TIMER_VALUE 0x0c

static void *digic_timer_base;

static uint64_t dummy_cs_read(void)
{
	return (uint64_t)(0xffff - readl(digic_timer_base + DIGIC_TIMER_VALUE));
}

static struct clocksource dummy_cs = {
	.read	= dummy_cs_read,
	.mask   = CLOCKSOURCE_MASK(16),
};

static int clocksource_init(void)
{
	clocks_calc_mult_shift(&dummy_cs.mult, &dummy_cs.shift,
		DIGIC_TIMER_CLOCK, NSEC_PER_SEC, 1);

	digic_timer_base = (void *)DIGIC4_TIMER2;

	/* disable timer */
	writel(0x80000000, digic_timer_base + DIGIC_TIMER_CONTROL);

	/* magic values... divider? */
	writel(0x00000002, digic_timer_base + 0x04);
	writel(0x00000003, digic_timer_base + 0x14);

	/* max counter value */
	writel(0x0000ffff, digic_timer_base + 0x08);

	init_clock(&dummy_cs);

	/* enable timer */
	writel(0x00000001, digic_timer_base + DIGIC_TIMER_CONTROL);
	/* start timer */
	writel(0x00000001, digic_timer_base + 0x10);

	return 0;
}
core_initcall(clocksource_init);
