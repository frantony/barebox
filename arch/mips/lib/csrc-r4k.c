/*
 * Copyright (C) 2011, 2015 Antony Pavlov <antonynpavlov@gmail.com>
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
#include <linux/clk.h>
#include <linux/err.h>
#include <clock.h>
#include <asm/mipsregs.h>

static uint64_t c0_hpt_read(void)
{
	return read_c0_count();
}

static struct clocksource mips_r4k_cs = {
	.read	= c0_hpt_read,
	.mask	= CLOCKSOURCE_MASK(32),
};

static int mips_r4k_timer_probe(struct device_d *dev)
{
	struct clk *timer_clk;
	unsigned long rate;

	timer_clk = clk_get(dev, NULL);
	if (IS_ERR(timer_clk)) {
		int ret = PTR_ERR(timer_clk);
		dev_err(dev, "clock not found: %d\n", ret);
		return ret;
	}

	rate = clk_get_rate(timer_clk);

	/* FIXME: we can use addition .compatible string for pipe half-rate counter */
	rate = rate / 2;

	clocks_calc_mult_shift(&mips_r4k_cs.mult, &mips_r4k_cs.shift,
		rate, NSEC_PER_SEC, 10);

	return init_clock(&mips_r4k_cs);
}

static __maybe_unused struct of_device_id mips_r4k_timer_dt_ids[] = {
	{
		.compatible = "mti,r4k-timer",
	}, {
		/* sentinel */
	}
};

static struct driver_d mips_r4k_timer_driver = {
	.probe	= mips_r4k_timer_probe,
	.name	= "mips-r4k-timer",
	.of_compatible = DRV_OF_COMPAT(mips_r4k_timer_dt_ids),
};

static int mips_r4k_clocksource_init(void)
{
	return platform_driver_register(&mips_r4k_timer_driver);
}
coredevice_initcall(mips_r4k_clocksource_init);
