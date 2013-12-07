/*
 * Copyright (C) 2011 Antony Pavlov <antonynpavlov@gmail.com>
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
 * @brief Clocksource based on MIPS CP0 timer
 */

#include <common.h>
#include <init.h>
#include <clock.h>
#include <io.h>
#include <asm/mipsregs.h>

unsigned int mips_hpt_frequency;

static uint64_t c0_hpt_read(void)
{
	return read_c0_count();
}

static struct clocksource cs = {
	.read	= c0_hpt_read,
	.mask	= 0xffffffff,
};

static int clocksource_init(void)
{
	if (!mips_hpt_frequency)
		mips_hpt_frequency = 100 * 1000 * 1000;

	pr_debug("csrc-r4k: mips_hpt_frequency=%d\n", mips_hpt_frequency);

	cs.mult = clocksource_hz2mult(mips_hpt_frequency, cs.shift);
	init_clock(&cs);

	return 0;
}
core_initcall(clocksource_init);
